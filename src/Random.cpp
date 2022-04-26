#include "components.hpp"


struct Random : Module {
	enum ParamIds {
		RATE_PARAM,
		SHAPE_PARAM,
		OFFSET_PARAM,
		MODE_PARAM, // removed in 2.0
		// new in 2.0
		PROB_PARAM,
		RAND_PARAM,
		RATE_CV_PARAM,
		SHAPE_CV_PARAM,
		PROB_CV_PARAM,
		RAND_CV_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RATE_INPUT,
		SHAPE_INPUT,
		TRIG_INPUT,
		EXTERNAL_INPUT,
		// new in 2.0
		PROB_INPUT,
		RAND_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		STEPPED_OUTPUT,
		LINEAR_OUTPUT,
		SMOOTH_OUTPUT,
		EXPONENTIAL_OUTPUT,
		// new in 2.0
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RATE_LIGHT,
		SHAPE_LIGHT,
		PROB_LIGHT,
		RAND_LIGHT,
		OFFSET_LIGHT,
		NUM_LIGHTS
	};

	float lastVoltage = 0.f;
	float nextVoltage = 0.f;
	/** Waits at 1 until reset */
	float phase = 0.f;
	float clockFreq = 0.f;
	/** Internal clock phase */
	float clockPhase = 0.f;
	/** External clock timer */
	dsp::Timer clockTimer;
	dsp::SchmittTrigger clockTrigger;
	dsp::PulseGenerator trigGenerator;

	Random() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RATE_PARAM, std::log2(0.002f), std::log2(2000.f), std::log2(2.f), "Clock rate", " Hz", 2);
		configParam(SHAPE_PARAM, 0.f, 1.f, 1.f, "Shape", "%", 0, 100);
		configParam(PROB_PARAM, 0.f, 1.f, 1.f, "Trigger probability", "%", 0, 100);
		configParam(RAND_PARAM, 0.f, 1.f, 1.f, "Random spread", "%", 0, 100);

		configParam(RATE_CV_PARAM, -1.f, 1.f, 0.f, "Clock rate CV", "%", 0, 100);
		getParamQuantity(RATE_CV_PARAM)->randomizeEnabled = false;
		configParam(SHAPE_CV_PARAM, -1.f, 1.f, 0.f, "Shape CV", "%", 0, 100);
		getParamQuantity(SHAPE_CV_PARAM)->randomizeEnabled = false;
		configParam(PROB_CV_PARAM, -1.f, 1.f, 0.f, "Trigger probability CV", "%", 0, 100);
		getParamQuantity(PROB_CV_PARAM)->randomizeEnabled = false;
		configParam(RAND_CV_PARAM, -1.f, 1.f, 0.f, "Random spread CV", "%", 0, 100);
		getParamQuantity(RAND_CV_PARAM)->randomizeEnabled = false;

		configSwitch(OFFSET_PARAM, 0.f, 1.f, 0.f, "Offset", {"Bipolar", "Unipolar"});

		configInput(RATE_INPUT, "Clock rate");
		configInput(SHAPE_INPUT, "Shape");
		configInput(PROB_INPUT, "Trigger probability");
		configInput(RAND_INPUT, "Random spread");
		configInput(TRIG_INPUT, "Trigger");
		configInput(EXTERNAL_INPUT, "External");

		configOutput(STEPPED_OUTPUT, "Stepped");
		configOutput(LINEAR_OUTPUT, "Linear");
		configOutput(SMOOTH_OUTPUT, "Smooth");
		configOutput(EXPONENTIAL_OUTPUT, "Exponential");
		configOutput(TRIG_OUTPUT, "Trigger");
	}

	void process(const ProcessArgs& args) override {
		// Params
		float shape = params[SHAPE_PARAM].getValue();
		shape += inputs[SHAPE_INPUT].getVoltage() / 10.f * params[SHAPE_CV_PARAM].getValue();
		shape = clamp(shape, 0.f, 1.f);

		float rand = params[RAND_PARAM].getValue();
		rand += inputs[RAND_INPUT].getVoltage() / 10.f * params[RAND_CV_PARAM].getValue();
		rand = clamp(rand, 0.f, 1.f);

		bool uni = params[OFFSET_PARAM].getValue() > 0.f;

		auto trigger = [&]() {
			float prob = params[PROB_PARAM].getValue();
			prob += inputs[PROB_INPUT].getVoltage() / 10.f * params[PROB_CV_PARAM].getValue();
			prob = clamp(prob, 0.f, 1.f);

			lights[RATE_LIGHT].setBrightness(3.f);

			// Probabilistic trigger
			if (prob < 1.f && random::uniform() > prob)
				return;

			// Generate next random voltage
			lastVoltage = nextVoltage;
			if (inputs[EXTERNAL_INPUT].isConnected()) {
				nextVoltage = inputs[EXTERNAL_INPUT].getVoltage();
			}
			else {
				// Crossfade new random voltage with last
				float v = 10.f * random::uniform();
				if (!uni)
					v -= 5.f;
				nextVoltage = crossfade(nextVoltage, v, rand);
			}

			phase = 0.f;

			trigGenerator.trigger(1e-3f);
			lights[PROB_LIGHT].setBrightness(3.f);
		};

		// Trigger or internal clock
		float deltaPhase;

		if (inputs[TRIG_INPUT].isConnected()) {
			clockTimer.process(args.sampleTime);

			if (clockTrigger.process(inputs[TRIG_INPUT].getVoltage(), 0.1f, 2.f)) {
				clockFreq = 1.f / clockTimer.getTime();
				clockTimer.reset();
				trigger();
			}

			deltaPhase = std::fmin(clockFreq * args.sampleTime, 0.5f);
		}
		else {
			// Advance clock phase by rate
			float rate = params[RATE_PARAM].getValue();
			rate += inputs[RATE_PARAM].getVoltage() * params[RATE_CV_PARAM].getValue();
			clockFreq = std::pow(2.f, rate);
			deltaPhase = std::fmin(clockFreq * args.sampleTime, 0.5f);
			clockPhase += deltaPhase;
			// Trigger
			if (clockPhase >= 1.f) {
				clockPhase -= 1.f;
				trigger();
			}
		}

		// Advance phase
		phase += deltaPhase;
		phase = std::min(1.f, phase);

		// Stepped
		if (outputs[STEPPED_OUTPUT].isConnected()) {
			float steps = std::ceil(std::pow(shape, 2) * 15 + 1);
			float v = std::ceil(phase * steps) / steps;
			v = rescale(v, 0.f, 1.f, lastVoltage, nextVoltage);
			outputs[STEPPED_OUTPUT].setVoltage(v);
		}

		// Linear
		if (outputs[LINEAR_OUTPUT].isConnected()) {
			float slope = 1 / shape;
			float v;
			if (slope < 1e6f) {
				v = std::fmin(phase * slope, 1.f);
			}
			else {
				v = 1.f;
			}
			v = rescale(v, 0.f, 1.f, lastVoltage, nextVoltage);
			outputs[LINEAR_OUTPUT].setVoltage(v);
		}

		// Smooth
		if (outputs[SMOOTH_OUTPUT].isConnected()) {
			float p = 1 / shape;
			float v;
			if (p < 1e6f) {
				v = std::fmin(phase * p, 1.f);
				v = std::cos(M_PI * v);
			}
			else {
				v = -1.f;
			}
			v = rescale(v, 1.f, -1.f, lastVoltage, nextVoltage);
			outputs[SMOOTH_OUTPUT].setVoltage(v);
		}

		// Exp
		if (outputs[EXPONENTIAL_OUTPUT].isConnected()) {
			float b = std::pow(shape, 8);
			float v;
			if (0.999f < b) {
				v = phase;
			}
			else if (1e-20f < b) {
				v = (std::pow(b, phase) - 1.f) / (b - 1.f);
			}
			else {
				v = 1.f;
			}
			v = rescale(v, 0.f, 1.f, lastVoltage, nextVoltage);
			outputs[EXPONENTIAL_OUTPUT].setVoltage(v);
		}

		// Trigger output
		outputs[TRIG_OUTPUT].setVoltage(trigGenerator.process(args.sampleTime) ? 10.f : 0.f);

		// Lights
		lights[RATE_LIGHT].setSmoothBrightness(0.f, args.sampleTime);
		lights[PROB_LIGHT].setSmoothBrightness(0.f, args.sampleTime);
		lights[RAND_LIGHT].setBrightness(rand);
		lights[SHAPE_LIGHT].setBrightness(shape);
		lights[OFFSET_LIGHT].setBrightness(uni);
	}

	void paramsFromJson(json_t* rootJ) override {
		// In <2.0, there were no attenuverters, so set them to 1.0 in case they are not overwritten.
		params[RATE_CV_PARAM].setValue(1.f);
		params[SHAPE_CV_PARAM].setValue(1.f);
		params[PROB_CV_PARAM].setValue(1.f);
		params[RAND_CV_PARAM].setValue(1.f);
		// In <2.0, mode=0 implied relative mode, corresponding to about 20% RND.
		params[RAND_PARAM].setValue(0.2f);
		Module::paramsFromJson(rootJ);

		// In <2.0, mode was used for absolute/relative mode. RND is a generalization so set it if mode is enabled.
		if (params[MODE_PARAM].getValue() > 0.f) {
			params[MODE_PARAM].setValue(0.f);
			params[RAND_PARAM].setValue(1.f);
		}
	}
};


struct RandomWidget : ModuleWidget {
	typedef CardinalBlackKnob<30> BigKnob;
	typedef CardinalBlackKnob<18> SmallKnob;

	static constexpr const int kWidth = 9;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kHorizontalPos1of3 = kBorderPadding + kUsableWidth * 0.1666f;
	static constexpr const float kHorizontalPos2of3 = kBorderPadding + kUsableWidth * 0.5f;
	static constexpr const float kHorizontalPos3of3 = kBorderPadding + kUsableWidth * 0.8333f;

	static constexpr const float kHorizontalPos1of2 = kBorderPadding + kUsableWidth * 0.333f;
	static constexpr const float kHorizontalPos2of2 = kBorderPadding + kUsableWidth * 0.666f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 309.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 248.f - BigKnob::kHalfSize;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 220.5f - SmallKnob::kHalfSize;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 192.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 145.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 72.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	RandomWidget(Random* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Random.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos1of3, kVerticalPos1), module, Random::TRIG_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos2of3, kVerticalPos1), module, Random::EXTERNAL_INPUT));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kHorizontalPos3of3, kVerticalPos1), module, Random::OFFSET_PARAM, Random::OFFSET_LIGHT));

		addParam(createParamCentered<BigKnob>(Vec(kHorizontalPos1of3, kVerticalPos2), module, Random::RATE_PARAM));
		addParam(createParamCentered<BigKnob>(Vec(kHorizontalPos2of3, kVerticalPos2), module, Random::RAND_PARAM));
		addParam(createParamCentered<BigKnob>(Vec(kHorizontalPos3of3, kVerticalPos2), module, Random::SHAPE_PARAM));

		addParam(createParamCentered<SmallKnob>(Vec(kHorizontalPos1of3, kVerticalPos3), module, Random::RATE_CV_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kHorizontalPos2of3, kVerticalPos3), module, Random::RAND_CV_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kHorizontalPos3of3, kVerticalPos3), module, Random::SHAPE_CV_PARAM));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos1of3, kVerticalPos4), module, Random::RATE_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos2of3, kVerticalPos4), module, Random::RAND_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos3of3, kVerticalPos4), module, Random::SHAPE_INPUT));

		addParam(createParamCentered<BigKnob>(Vec(kHorizontalPos1of3, kVerticalPos5), module, Random::PROB_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kHorizontalPos2of3, kVerticalPos5), module, Random::PROB_CV_PARAM));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos3of3, kVerticalPos5), module, Random::PROB_INPUT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos1of3, kVerticalPos6), module, Random::STEPPED_OUTPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos2of3, kVerticalPos6), module, Random::LINEAR_OUTPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos3of3, kVerticalPos6), module, Random::EXPONENTIAL_OUTPUT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos1of2, kVerticalPos7), module, Random::SMOOTH_OUTPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos2of2, kVerticalPos7), module, Random::TRIG_OUTPUT));
	}
};


Model* modelRandom = createModel<Random, RandomWidget>("Random");
