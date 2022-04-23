#include "components.hpp"
#include <samplerate.h>


struct Delay : Module {
	enum ParamId {
		TIME_PARAM,
		FEEDBACK_PARAM,
		TONE_PARAM,
		MIX_PARAM,
		// new in 2.0
		TIME_CV_PARAM,
		FEEDBACK_CV_PARAM,
		TONE_CV_PARAM,
		MIX_CV_PARAM,
		NUM_PARAMS
	};
	enum InputId {
		TIME_INPUT,
		FEEDBACK_INPUT,
		TONE_INPUT,
		MIX_INPUT,
		IN_INPUT,
		// new in 2.0
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputId {
		MIX_OUTPUT,
		// new in 2.0
		WET_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightId {
		CLOCK_LIGHT,
		NUM_LIGHTS
	};

	constexpr static size_t HISTORY_SIZE = 1 << 21;
	dsp::DoubleRingBuffer<float, HISTORY_SIZE> historyBuffer;
	dsp::DoubleRingBuffer<float, 16> outBuffer;
	SRC_STATE* src;
	float lastWet = 0.f;
	dsp::RCFilter lowpassFilter;
	dsp::RCFilter highpassFilter;
	float clockFreq = 1.f;
	dsp::Timer clockTimer;
	dsp::SchmittTrigger clockTrigger;

	Delay() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// This was made before the pitch voltage standard existed, so it uses TIME_PARAM = 0 as 0.001s and TIME_PARAM = 1 as 10s with a formula of:
		// time = 0.001 * 10000^TIME_PARAM
		// or
		// TIME_PARAM = log10(time * 1000) / 4
		const float timeMin = log10(0.001f * 1000) / 4;
		const float timeMax = log10(10.f * 1000) / 4;
		const float timeDefault = log10(0.5f * 1000) / 4;
		configParam(TIME_PARAM, timeMin, timeMax, timeDefault, "Time", " s", 10.f / 1e-3, 1e-3);
		configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "Feedback", "%", 0, 100);
		configParam(TONE_PARAM, 0.f, 1.f, 0.5f, "Tone", "%", 0, 200, -100);
		configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix", "%", 0, 100);
		configParam(TIME_CV_PARAM, -1.f, 1.f, 0.f, "Time CV", "%", 0, 100);
		getParamQuantity(TIME_CV_PARAM)->randomizeEnabled = false;
		configParam(FEEDBACK_CV_PARAM, -1.f, 1.f, 0.f, "Feedback CV", "%", 0, 100);
		getParamQuantity(FEEDBACK_CV_PARAM)->randomizeEnabled = false;
		configParam(TONE_CV_PARAM, -1.f, 1.f, 0.f, "Tone CV", "%", 0, 100);
		getParamQuantity(TONE_CV_PARAM)->randomizeEnabled = false;
		configParam(MIX_CV_PARAM, -1.f, 1.f, 0.f, "Mix CV", "%", 0, 100);
		getParamQuantity(MIX_CV_PARAM)->randomizeEnabled = false;

		configInput(TIME_INPUT, "Time");
		getInputInfo(TIME_INPUT)->description = "1V/octave when Time CV is 100%";
		configInput(FEEDBACK_INPUT, "Feedback");
		configInput(TONE_INPUT, "Tone");
		configInput(MIX_INPUT, "Mix");
		configInput(IN_INPUT, "Audio");
		configInput(CLOCK_INPUT, "Clock");

		configOutput(MIX_OUTPUT, "Mix");
		configOutput(WET_OUTPUT, "Wet");

		configBypass(IN_INPUT, WET_OUTPUT);
		configBypass(IN_INPUT, MIX_OUTPUT);

		src = src_new(SRC_SINC_FASTEST, 1, NULL);
		assert(src);
	}

	~Delay() {
		src_delete(src);
	}

	void process(const ProcessArgs& args) override {
		// Clock
		if (inputs[CLOCK_INPUT].isConnected()) {
			clockTimer.process(args.sampleTime);

			if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
				float clockFreq = 1.f / clockTimer.getTime();
				clockTimer.reset();
				if (0.001f <= clockFreq && clockFreq <= 1000.f) {
					this->clockFreq = clockFreq;
				}
			}
		}
		else {
			// Default frequency when clock is unpatched
			clockFreq = 2.f;
		}

		// Get input to delay block
		float in = inputs[IN_INPUT].getVoltageSum();
		float feedback = params[FEEDBACK_PARAM].getValue() + inputs[FEEDBACK_INPUT].getVoltage() / 10.f * params[FEEDBACK_CV_PARAM].getValue();
		feedback = clamp(feedback, 0.f, 1.f);
		float dry = in + lastWet * feedback;

		// Compute freq
		// Scale time knob to 1V/oct pitch based on formula explained in constructor, for backwards compatibility
		float pitch = std::log2(1000.f) - std::log2(10000.f) * params[TIME_PARAM].getValue();
		pitch += inputs[TIME_INPUT].getVoltage() * params[TIME_CV_PARAM].getValue();
		float freq = clockFreq / 2.f * std::pow(2.f, pitch);
		// Number of desired delay samples
		float index = args.sampleRate / freq;
		// In order to delay accurate samples, subtract by the historyBuffer size, and an experimentally tweaked amount.
		index -= 16 + 4.f;
		index = clamp(index, 2.f, float(HISTORY_SIZE - 1));
		// DEBUG("freq %f index %f", freq, index);


		// Push dry sample into history buffer
		if (!historyBuffer.full()) {
			historyBuffer.push(dry);
		}

		if (outBuffer.empty()) {
			// How many samples do we need consume to catch up?
			float consume = index - historyBuffer.size();
			double ratio = std::pow(4.f, clamp(consume / 10000.f, -1.f, 1.f));
			// DEBUG("index %f historyBuffer %lu consume %f ratio %lf", index, historyBuffer.size(), consume, ratio);

			// Convert samples from the historyBuffer to catch up or slow down so `index` and `historyBuffer.size()` eventually match approximately
			SRC_DATA srcData;
			srcData.data_in = (const float*) historyBuffer.startData();
			srcData.data_out = (float*) outBuffer.endData();
			srcData.input_frames = std::min((int) historyBuffer.size(), 16);
			srcData.output_frames = outBuffer.capacity();
			srcData.end_of_input = false;
			srcData.src_ratio = ratio;
			src_process(src, &srcData);
			historyBuffer.startIncr(srcData.input_frames_used);
			outBuffer.endIncr(srcData.output_frames_gen);
			// DEBUG("used %ld gen %ld", srcData.input_frames_used, srcData.output_frames_gen);
		}

		float wet = 0.f;
		if (!outBuffer.empty()) {
			wet = outBuffer.shift();
		}

		// Apply color to delay wet output
		float color = params[TONE_PARAM].getValue() + inputs[TONE_INPUT].getVoltage() / 10.f * params[TONE_CV_PARAM].getValue();
		color = clamp(color, 0.f, 1.f);
		float colorFreq = std::pow(100.f, 2.f * color - 1.f);

		float lowpassFreq = clamp(20000.f * colorFreq, 20.f, 20000.f);
		lowpassFilter.setCutoffFreq(lowpassFreq / args.sampleRate);
		lowpassFilter.process(wet);
		wet = lowpassFilter.lowpass();

		float highpassFreq = clamp(20.f * colorFreq, 20.f, 20000.f);
		highpassFilter.setCutoff(highpassFreq / args.sampleRate);
		highpassFilter.process(wet);
		wet = highpassFilter.highpass();

		// Set wet output
		outputs[WET_OUTPUT].setVoltage(wet);
		lastWet = wet;

		// Set mix output
		float mix = params[MIX_PARAM].getValue() + inputs[MIX_INPUT].getVoltage() / 10.f * params[MIX_CV_PARAM].getValue();
		mix = clamp(mix, 0.f, 1.f);
		float out = crossfade(in, wet, mix);
		outputs[MIX_OUTPUT].setVoltage(out);
	}

	void paramsFromJson(json_t* rootJ) override {
		// These attenuators didn't exist in version <2.0, so set to 1 in case they are not overwritten.
		params[FEEDBACK_CV_PARAM].setValue(1.f);
		params[TONE_CV_PARAM].setValue(1.f);
		params[MIX_CV_PARAM].setValue(1.f);
		// The time input scaling has changed, so don't set to 1.
		// params[TIME_CV_PARAM].setValue(1.f);

		Module::paramsFromJson(rootJ);
	}
};


struct DelayWidget : ModuleWidget {
	static constexpr const int kWidth = 9;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.25f;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.75f;

	typedef CardinalBlackKnob<40> BigKnob;
	typedef CardinalBlackKnob<18> SmallKnob;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 312.f - 11.f;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 248.f - BigKnob::kHalfSize;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 224.5f - SmallKnob::kHalfSize;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 197.f - 11.f;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 126.f - BigKnob::kHalfSize;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 102.f - SmallKnob::kHalfSize;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 77.f - 11.f;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 26.f - 11.f;

	DelayWidget(Delay* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Delay.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos1), module, Delay::IN_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, Delay::CLOCK_INPUT));

		addParam(createParamCentered<BigKnob>(Vec(kPosLeft, kVerticalPos2), module, Delay::TIME_PARAM));
		addParam(createParamCentered<BigKnob>(Vec(kPosRight, kVerticalPos2), module, Delay::FEEDBACK_PARAM));

		addParam(createParamCentered<SmallKnob>(Vec(kPosLeft, kVerticalPos3), module, Delay::TIME_CV_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kPosRight, kVerticalPos3), module, Delay::FEEDBACK_CV_PARAM));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos4), module, Delay::TIME_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, Delay::FEEDBACK_INPUT));

		addParam(createParamCentered<BigKnob>(Vec(kPosLeft, kVerticalPos5), module, Delay::TONE_PARAM));
		addParam(createParamCentered<BigKnob>(Vec(kPosRight, kVerticalPos5), module, Delay::MIX_PARAM));

		addParam(createParamCentered<SmallKnob>(Vec(kPosLeft, kVerticalPos6), module, Delay::TONE_CV_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kPosRight, kVerticalPos6), module, Delay::MIX_CV_PARAM));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos7), module, Delay::TONE_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos7), module, Delay::MIX_INPUT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos8), module, Delay::WET_OUTPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos8), module, Delay::MIX_OUTPUT));
	}
};


Model* modelDelay = createModel<Delay, DelayWidget>("Delay");
