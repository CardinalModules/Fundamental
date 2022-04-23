#include "components.hpp"


struct Mutes : Module {
	enum ParamIds {
		ENUMS(MUTE_PARAMS, 10),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(IN_INPUTS, 10),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(OUT_OUTPUTS, 10),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(MUTE_LIGHTS, 10),
		NUM_LIGHTS
	};

	Mutes() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 10; i++) {
			configSwitch(MUTE_PARAMS + i, 0.f, 1.f, 0.f, string::f("Row %d mute", i + 1));
			configInput(IN_INPUTS + i, string::f("Row %d", i + 1));
			configOutput(OUT_OUTPUTS + i, string::f("Row %d", i + 1));
		}
	}

	void process(const ProcessArgs& args) override {
		const float zero[16] = {};
		float out[16] = {};

		// Iterate rows
		for (int i = 0; i < 10; i++) {
			int channels = 1;
			bool mute = params[MUTE_PARAMS + i].getValue() > 0.f;

			// Get input
			// Inputs are normalized to the input above it, so only set if connected
			if (inputs[IN_INPUTS + i].isConnected()) {
				channels = inputs[IN_INPUTS + i].getChannels();
				inputs[IN_INPUTS + i].readVoltages(out);
			}

			// Set output
			if (outputs[OUT_OUTPUTS + i].isConnected()) {
				outputs[OUT_OUTPUTS + i].setChannels(channels);
				outputs[OUT_OUTPUTS + i].writeVoltages(mute ? zero : out);
			}

			// Set light
			lights[MUTE_LIGHTS + i].setBrightness(mute);
		}
	}

	void dataFromJson(json_t* rootJ) override {
		// In <2.0, states were stored in data
		json_t* statesJ = json_object_get(rootJ, "states");
		if (statesJ) {
			for (int i = 0; i < 10; i++) {
				json_t* stateJ = json_array_get(statesJ, i);
				if (stateJ)
					params[MUTE_PARAMS + i].setValue(!json_boolean_value(stateJ));
			}
		}
	}

	void invert() {
		for (int i = 0; i < 10; i++) {
			params[MUTE_PARAMS + i].setValue(!params[MUTE_PARAMS + i].getValue());
		}
	}
};


struct MutesWidget : ModuleWidget {
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

	MutesWidget(Mutes* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Mutes.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos1), module, Mutes::IN_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos2), module, Mutes::IN_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos3), module, Mutes::IN_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos4), module, Mutes::IN_INPUTS + 3));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos5), module, Mutes::IN_INPUTS + 4));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos6), module, Mutes::IN_INPUTS + 5));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos7), module, Mutes::IN_INPUTS + 6));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos8), module, Mutes::IN_INPUTS + 7));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos9), module, Mutes::IN_INPUTS + 8));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos10), module, Mutes::IN_INPUTS + 9));

		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos1), module, Mutes::MUTE_PARAMS + 0, Mutes::MUTE_LIGHTS + 0));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos2), module, Mutes::MUTE_PARAMS + 1, Mutes::MUTE_LIGHTS + 1));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos3), module, Mutes::MUTE_PARAMS + 2, Mutes::MUTE_LIGHTS + 2));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos4), module, Mutes::MUTE_PARAMS + 3, Mutes::MUTE_LIGHTS + 3));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos5), module, Mutes::MUTE_PARAMS + 4, Mutes::MUTE_LIGHTS + 4));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos6), module, Mutes::MUTE_PARAMS + 5, Mutes::MUTE_LIGHTS + 5));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos7), module, Mutes::MUTE_PARAMS + 6, Mutes::MUTE_LIGHTS + 6));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos8), module, Mutes::MUTE_PARAMS + 7, Mutes::MUTE_LIGHTS + 7));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos9), module, Mutes::MUTE_PARAMS + 8, Mutes::MUTE_LIGHTS + 8));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosCenter, kVerticalPos10), module, Mutes::MUTE_PARAMS + 9, Mutes::MUTE_LIGHTS + 9));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, Mutes::OUT_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos2), module, Mutes::OUT_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos3), module, Mutes::OUT_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, Mutes::OUT_OUTPUTS + 3));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos5), module, Mutes::OUT_OUTPUTS + 4));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos6), module, Mutes::OUT_OUTPUTS + 5));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos7), module, Mutes::OUT_OUTPUTS + 6));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos8), module, Mutes::OUT_OUTPUTS + 7));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos9), module, Mutes::OUT_OUTPUTS + 8));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos10), module, Mutes::OUT_OUTPUTS + 9));
	}

	void appendContextMenu(Menu* menu) override {
		Mutes* module = dynamic_cast<Mutes*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createMenuItem("Invert mutes", "",
			[=]() {module->invert();}
		));
	}
};


Model* modelMutes = createModel<Mutes, MutesWidget>("Mutes");
