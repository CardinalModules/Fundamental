#include "components.hpp"


struct Pulses : Module {
	enum ParamIds {
		ENUMS(PUSH_PARAMS, 10),
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(TRIG_OUTPUTS, 10),
		ENUMS(GATE_OUTPUTS, 10),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PUSH_LIGHTS, 10),
		NUM_LIGHTS
	};

	dsp::BooleanTrigger tapTriggers[10];
	dsp::PulseGenerator pulseGenerators[10];

	Pulses() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 10; i++) {
			configButton(PUSH_PARAMS + i, string::f("Row %d push", i + 1));
			configOutput(TRIG_OUTPUTS + i, string::f("Row %d trigger", i + 1));
			configOutput(GATE_OUTPUTS + i, string::f("Row %d gate", i + 1));
		}
	}

	void process(const ProcessArgs& args) override {
		for (int i = 0; i < 10; i++) {
			bool tap = params[PUSH_PARAMS + i].getValue() > 0.f;
			if (tapTriggers[i].process(tap)) {
				pulseGenerators[i].trigger(1e-3f);
			}
			bool pulse = pulseGenerators[i].process(args.sampleTime);
			outputs[TRIG_OUTPUTS + i].setVoltage(pulse ? 10.f : 0.f);
			outputs[GATE_OUTPUTS + i].setVoltage(tap ? 10.f : 0.f);
			lights[PUSH_LIGHTS + i].setBrightness(tap);
		}
	}
};


struct PulsesWidget : ModuleWidget {
	static constexpr const int kWidth = 8;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.1333f;
	static constexpr const float kPosCenter = kBorderPadding + kUsableWidth * 0.5f;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.8333f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 301.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 271.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 241.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 211.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 181.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 151.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 121.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 91.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos9 = kRACK_GRID_HEIGHT - 61.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos10 = kRACK_GRID_HEIGHT - 31.f - kRACK_JACK_HALF_SIZE;

	PulsesWidget(Pulses* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Pulses.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos1), module, Pulses::PUSH_PARAMS + 0, Pulses::PUSH_LIGHTS + 0));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos2), module, Pulses::PUSH_PARAMS + 1, Pulses::PUSH_LIGHTS + 1));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos3), module, Pulses::PUSH_PARAMS + 2, Pulses::PUSH_LIGHTS + 2));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos4), module, Pulses::PUSH_PARAMS + 3, Pulses::PUSH_LIGHTS + 3));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos5), module, Pulses::PUSH_PARAMS + 4, Pulses::PUSH_LIGHTS + 4));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos6), module, Pulses::PUSH_PARAMS + 5, Pulses::PUSH_LIGHTS + 5));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos7), module, Pulses::PUSH_PARAMS + 6, Pulses::PUSH_LIGHTS + 6));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos8), module, Pulses::PUSH_PARAMS + 7, Pulses::PUSH_LIGHTS + 7));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos9), module, Pulses::PUSH_PARAMS + 8, Pulses::PUSH_LIGHTS + 8));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos10), module, Pulses::PUSH_PARAMS + 9, Pulses::PUSH_LIGHTS + 9));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos1), module, Pulses::TRIG_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos2), module, Pulses::TRIG_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos3), module, Pulses::TRIG_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos4), module, Pulses::TRIG_OUTPUTS + 3));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos5), module, Pulses::TRIG_OUTPUTS + 4));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos6), module, Pulses::TRIG_OUTPUTS + 5));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos7), module, Pulses::TRIG_OUTPUTS + 6));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos8), module, Pulses::TRIG_OUTPUTS + 7));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos9), module, Pulses::TRIG_OUTPUTS + 8));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos10), module, Pulses::TRIG_OUTPUTS + 9));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, Pulses::GATE_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos2), module, Pulses::GATE_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos3), module, Pulses::GATE_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, Pulses::GATE_OUTPUTS + 3));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos5), module, Pulses::GATE_OUTPUTS + 4));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos6), module, Pulses::GATE_OUTPUTS + 5));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos7), module, Pulses::GATE_OUTPUTS + 6));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos8), module, Pulses::GATE_OUTPUTS + 7));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos9), module, Pulses::GATE_OUTPUTS + 8));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos10), module, Pulses::GATE_OUTPUTS + 9));

	}
};


Model* modelPulses = createModel<Pulses, PulsesWidget>("Pulses");
