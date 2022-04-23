#include "components.hpp"


struct Merge : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(MONO_INPUTS, 16),
		NUM_INPUTS
	};
	enum OutputIds {
		POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(CHANNEL_LIGHTS, 16),
		NUM_LIGHTS
	};

	dsp::ClockDivider lightDivider;
	int channels = -1;
	int automaticChannels = 0;

	Merge() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		for (int i = 0; i < 16; i++)
			configInput(MONO_INPUTS + i, string::f("Channel %d", i + 1));
		configOutput(POLY_OUTPUT, "Polyphonic");

		lightDivider.setDivision(512);
		onReset();
	}

	void onReset() override {
		channels = -1;
	}

	void process(const ProcessArgs& args) override {
		int lastChannel = -1;
		for (int c = 0; c < 16; c++) {
			float v = 0.f;
			if (inputs[MONO_INPUTS + c].isConnected()) {
				lastChannel = c;
				v = inputs[MONO_INPUTS + c].getVoltage();
			}
			outputs[POLY_OUTPUT].setVoltage(v, c);
		}
		automaticChannels = lastChannel + 1;

		// In order to allow 0 channels, modify `channels` directly instead of using `setChannels()`
		outputs[POLY_OUTPUT].channels = (channels >= 0) ? channels : automaticChannels;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "channels", json_integer(channels));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* channelsJ = json_object_get(rootJ, "channels");
		if (channelsJ)
			channels = json_integer_value(channelsJ);
	}
};


struct MergeWidget : ModuleWidget {
	static constexpr const int kWidth = 5;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.25f;
	static constexpr const float kPosCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.75f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 293.f - 11.f;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 264.f - 11.f;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 235.f - 11.f;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 206.f - 11.f;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 177.f - 11.f;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 148.f - 11.f;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 119.f - 11.f;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 90.f - 11.f;

	MergeWidget(Merge* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Merge.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos1), module, Merge::MONO_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos2), module, Merge::MONO_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos3), module, Merge::MONO_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos4), module, Merge::MONO_INPUTS + 3));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos5), module, Merge::MONO_INPUTS + 4));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos6), module, Merge::MONO_INPUTS + 5));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos7), module, Merge::MONO_INPUTS + 6));
		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos8), module, Merge::MONO_INPUTS + 7));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, Merge::MONO_INPUTS + 8));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos2), module, Merge::MONO_INPUTS + 9));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos3), module, Merge::MONO_INPUTS + 10));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, Merge::MONO_INPUTS + 11));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos5), module, Merge::MONO_INPUTS + 12));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos6), module, Merge::MONO_INPUTS + 13));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos7), module, Merge::MONO_INPUTS + 14));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos8), module, Merge::MONO_INPUTS + 15));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kRACK_GRID_HEIGHT - 26.f - 11.f), module, Merge::POLY_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Merge* module = dynamic_cast<Merge*>(this->module);

		menu->addChild(new MenuSeparator);

		std::vector<std::string> channelLabels;
		channelLabels.push_back(string::f("Automatic (%d)", module->automaticChannels));
		for (int i = 0; i <= 16; i++) {
			channelLabels.push_back(string::f("%d", i));
		}
		menu->addChild(createIndexSubmenuItem("Channels", channelLabels,
			[=]() {return module->channels + 1;},
			[=](int i) {module->channels = i - 1;}
		));
	}
};


Model* modelMerge = createModel<Merge, MergeWidget>("Merge");
