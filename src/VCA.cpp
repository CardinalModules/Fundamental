#include "components.hpp"


// Deprecated. Use VCA-1 instead.


struct VCA : Module {
	enum ParamIds {
		LEVEL1_PARAM,
		LEVEL2_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		EXP1_INPUT,
		LIN1_INPUT,
		IN1_INPUT,
		EXP2_INPUT,
		LIN2_INPUT,
		IN2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT1_OUTPUT,
		OUT2_OUTPUT,
		NUM_OUTPUTS
	};

	VCA() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(LEVEL1_PARAM, 0.0, 1.0, 1.0, "Channel 1 level", "%", 0, 100);
		configParam(LEVEL2_PARAM, 0.0, 1.0, 1.0, "Channel 2 level", "%", 0, 100);
		configInput(EXP1_INPUT, "Channel 1 exponential CV");
		configInput(EXP2_INPUT, "Channel 2 exponential CV");
		configInput(LIN1_INPUT, "Channel 1 linear CV");
		configInput(LIN2_INPUT, "Channel 2 linear CV");
		configInput(IN1_INPUT, "Channel 1");
		configInput(IN2_INPUT, "Channel 2");
		configOutput(OUT1_OUTPUT, "Channel 1");
		configOutput(OUT2_OUTPUT, "Channel 2");
		configBypass(IN1_INPUT, OUT1_OUTPUT);
		configBypass(IN2_INPUT, OUT2_OUTPUT);
	}

	void processChannel(Input& in, Param& level, Input& lin, Input& exp, Output& out) {
		// Get input
		int channels = std::max(in.getChannels(), 1);
		simd::float_4 v[4];
		for (int c = 0; c < channels; c += 4) {
			v[c / 4] = simd::float_4::load(in.getVoltages(c));
		}

		// Apply knob gain
		float gain = level.getValue();
		for (int c = 0; c < channels; c += 4) {
			v[c / 4] *= gain;
		}

		// Apply linear CV gain
		if (lin.isConnected()) {
			if (lin.isPolyphonic()) {
				for (int c = 0; c < channels; c += 4) {
					simd::float_4 cv = simd::float_4::load(lin.getVoltages(c)) / 10.f;
					cv = clamp(cv, 0.f, 1.f);
					v[c / 4] *= cv;
				}
			}
			else {
				float cv = lin.getVoltage() / 10.f;
				cv = clamp(cv, 0.f, 1.f);
				for (int c = 0; c < channels; c += 4) {
					v[c / 4] *= cv;
				}
			}
		}

		// Apply exponential CV gain
		const float expBase = 50.f;
		if (exp.isConnected()) {
			if (exp.isPolyphonic()) {
				for (int c = 0; c < channels; c += 4) {
					simd::float_4 cv = simd::float_4::load(exp.getVoltages(c)) / 10.f;
					cv = clamp(cv, 0.f, 1.f);
					cv = rescale(pow(expBase, cv), 1.f, expBase, 0.f, 1.f);
					v[c / 4] *= cv;
				}
			}
			else {
				float cv = exp.getVoltage() / 10.f;
				cv = clamp(cv, 0.f, 1.f);
				cv = rescale(std::pow(expBase, cv), 1.f, expBase, 0.f, 1.f);
				for (int c = 0; c < channels; c += 4) {
					v[c / 4] *= cv;
				}
			}
		}

		// Set output
		out.setChannels(channels);
		for (int c = 0; c < channels; c += 4) {
			v[c / 4].store(out.getVoltages(c));
		}
	}

	void process(const ProcessArgs& args) override {
		processChannel(inputs[IN1_INPUT], params[LEVEL1_PARAM], inputs[LIN1_INPUT], inputs[EXP1_INPUT], outputs[OUT1_OUTPUT]);
		processChannel(inputs[IN2_INPUT], params[LEVEL2_PARAM], inputs[LIN2_INPUT], inputs[EXP2_INPUT], outputs[OUT2_OUTPUT]);
	}
};



struct VCAWidget : ModuleWidget {
	typedef CardinalBlackKnob<35> Knob;

	static constexpr const int kWidth = 5;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kOffsetX = 11.f; // FIXME
	static constexpr const float kOffsetY = 11.f; // FIXME
	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.25f - kOffsetX;
	static constexpr const float kPosCenter = kBorderPadding + kUsableWidth * 0.5f - kOffsetX;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.75f - kOffsetX;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 312.f - kRACK_JACK_HALF_SIZE - kOffsetY;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 280.f - kRACK_JACK_HALF_SIZE - kOffsetY;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 233.f - Knob::kHalfSize - kOffsetY*2;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 197.f - kRACK_JACK_HALF_SIZE - kOffsetY;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 140.f - kRACK_JACK_HALF_SIZE - kOffsetY;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 108.f - kRACK_JACK_HALF_SIZE - kOffsetY;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 61.f - Knob::kHalfSize - kOffsetY*2;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE - kOffsetY;

	VCAWidget(VCA* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VCA.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInput<CardinalPort>(Vec(kPosCenter, kVerticalPos1), module, VCA::IN1_INPUT));
		addInput(createInput<CardinalPort>(Vec(kPosLeft, kVerticalPos2), module, VCA::EXP1_INPUT));
		addInput(createInput<CardinalPort>(Vec(kPosRight, kVerticalPos2), module, VCA::LIN1_INPUT));
		addParam(createParam<Knob>(Vec(kPosCenter - 7.f /* FIXME */, kVerticalPos3), module, VCA::LEVEL1_PARAM));
		addOutput(createOutput<CardinalPort>(Vec(kPosCenter, kVerticalPos4), module, VCA::OUT1_OUTPUT));

		addInput(createInput<CardinalPort>(Vec(kPosCenter, kVerticalPos5), module, VCA::IN2_INPUT));
		addInput(createInput<CardinalPort>(Vec(kPosLeft, kVerticalPos6), module, VCA::EXP2_INPUT));
		addInput(createInput<CardinalPort>(Vec(kPosRight, kVerticalPos6), module, VCA::LIN2_INPUT));
		addParam(createParam<Knob>(Vec(kPosCenter - 7.f /* FIXME */, kVerticalPos7), module, VCA::LEVEL2_PARAM));
		addOutput(createOutput<CardinalPort>(Vec(kPosCenter, kVerticalPos8), module, VCA::OUT2_OUTPUT));
	}
};


Model* modelVCA = createModel<VCA, VCAWidget>("VCA");
