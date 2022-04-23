#include "components.hpp"


struct _8vert : Module {
	enum ParamIds {
		ENUMS(GAIN_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IN_INPUTS, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	dsp::ClockDivider paramDivider;

	_8vert() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 8; i++) {
			configParam(GAIN_PARAMS + i, -1.f, 1.f, 0.f, string::f("Row %d gain", i + 1), "%", 0, 100);
			configInput(IN_INPUTS + i, string::f("Row %d", i + 1));
			configOutput(OUT_OUTPUTS + i, string::f("Row %d", i + 1));
		}

		paramDivider.setDivision(2048);
	}

	void process(const ProcessArgs& args) override {
		float in[16] = {10.f};
		int channels = 1;

		for (int i = 0; i < 8; i++) {
			// Get input
			if (inputs[IN_INPUTS + i].isConnected()) {
				channels = inputs[IN_INPUTS + i].getChannels();
				inputs[IN_INPUTS + i].readVoltages(in);
			}

			if (outputs[OUT_OUTPUTS + i].isConnected()) {
				// Apply gain
				float out[16];
				float gain = params[GAIN_PARAMS + i].getValue();
				for (int c = 0; c < channels; c++) {
					out[c] = gain * in[c];
				}

				// Set output
				outputs[OUT_OUTPUTS + i].setChannels(channels);
				outputs[OUT_OUTPUTS + i].writeVoltages(out);
			}
		}

		if (paramDivider.process()) {
			refreshParamQuantities();
		}
	}

	/** Set the gain param units to either V or %, depending on whether a cable is connected. */
	void refreshParamQuantities() {
		bool normalized = true;

		for (int i = 0; i < 8; i++) {
			ParamQuantity* pq = paramQuantities[GAIN_PARAMS + i];
			if (!pq)
				continue;

			if (inputs[IN_INPUTS + i].isConnected())
				normalized = false;
			if (normalized) {
				pq->unit = "V";
				pq->displayMultiplier = 10.f;
			}
			else {
				pq->unit = "%";
				pq->displayMultiplier = 100.f;
			}
		}
	}
};


struct _8vertWidget : ModuleWidget {
	static constexpr const int kWidth = 8;
	static constexpr const float kPosRight = 83.f + 16.f;
	static constexpr const float kPosCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;
	static constexpr const float kPosLeft = kRACK_GRID_WIDTH * kWidth - kPosRight;
	static constexpr const float kParam0Y = kRACK_GRID_HEIGHT - 292.f - 17.f;
	static constexpr const float kParam1Y = kRACK_GRID_HEIGHT - 254.f - 17.f;
	static constexpr const float kParam2Y = kRACK_GRID_HEIGHT - 216.f - 17.f;
	static constexpr const float kParam3Y = kRACK_GRID_HEIGHT - 178.f - 17.f;
	static constexpr const float kParam4Y = kRACK_GRID_HEIGHT - 140.f - 17.f;
	static constexpr const float kParam5Y = kRACK_GRID_HEIGHT - 102.f - 17.f;
	static constexpr const float kParam6Y = kRACK_GRID_HEIGHT - 64.f - 17.f;
	static constexpr const float kParam7Y = kRACK_GRID_HEIGHT - 26.f - 17.f;

	typedef CardinalBlackKnob<30> Knob;

	_8vertWidget(_8vert* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/8vert.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam0Y), module, _8vert::GAIN_PARAMS + 0));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam1Y), module, _8vert::GAIN_PARAMS + 1));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam2Y), module, _8vert::GAIN_PARAMS + 2));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam3Y), module, _8vert::GAIN_PARAMS + 3));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam4Y), module, _8vert::GAIN_PARAMS + 4));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam5Y), module, _8vert::GAIN_PARAMS + 5));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam6Y), module, _8vert::GAIN_PARAMS + 6));
		addParam(createParamCentered<Knob>(Vec(kPosCenter, kParam7Y), module, _8vert::GAIN_PARAMS + 7));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam0Y), module, _8vert::IN_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam1Y), module, _8vert::IN_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam2Y), module, _8vert::IN_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam3Y), module, _8vert::IN_INPUTS + 3));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam4Y), module, _8vert::IN_INPUTS + 4));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam5Y), module, _8vert::IN_INPUTS + 5));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam6Y), module, _8vert::IN_INPUTS + 6));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kParam7Y), module, _8vert::IN_INPUTS + 7));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam0Y), module, _8vert::OUT_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam1Y), module, _8vert::OUT_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam2Y), module, _8vert::OUT_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam3Y), module, _8vert::OUT_OUTPUTS + 3));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam4Y), module, _8vert::OUT_OUTPUTS + 4));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam5Y), module, _8vert::OUT_OUTPUTS + 5));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam6Y), module, _8vert::OUT_OUTPUTS + 6));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kParam7Y), module, _8vert::OUT_OUTPUTS + 7));
	}
};


Model* model_8vert = createModel<_8vert, _8vertWidget>("8vert");
