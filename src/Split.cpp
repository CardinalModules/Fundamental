#include "components.hpp"


struct Split : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		POLY_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MONO_OUTPUTS, 16),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(CHANNEL_LIGHTS, 16),
		NUM_LIGHTS
	};

	int lastChannels = 0;
	dsp::ClockDivider lightDivider;

	Split() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configInput(POLY_INPUT, "Polyphonic");
		for (int i = 0; i < 16; i++)
			configOutput(MONO_OUTPUTS + i, string::f("Channel %d", i + 1));

		lightDivider.setDivision(512);
	}

	void process(const ProcessArgs& args) override {
		for (int c = 0; c < 16; c++) {
			float v = inputs[POLY_INPUT].getVoltage(c);
			// To allow users to debug buggy modules, don't assume that undefined channel voltages are 0V.
			outputs[MONO_OUTPUTS + c].setVoltage(v);
		}

		lastChannels = inputs[POLY_INPUT].getChannels();
	}
};


struct SplitWidget : ModuleWidget {
	static constexpr const int kWidth = 5;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.25f;
	static constexpr const float kPosCenter = kBorderPadding + kUsableWidth * 0.5f;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.75f;

	static constexpr const float kVerticalPosIn = kRACK_GRID_HEIGHT - 300.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 236.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 207.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 178.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 149.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 120.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 91.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 62.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 33.f - kRACK_JACK_HALF_SIZE;

	SplitWidget(Split* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Split.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPosIn), module, Split::POLY_INPUT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos1), module, Split::MONO_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos2), module, Split::MONO_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos3), module, Split::MONO_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos4), module, Split::MONO_OUTPUTS + 3));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos5), module, Split::MONO_OUTPUTS + 4));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos6), module, Split::MONO_OUTPUTS + 5));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos7), module, Split::MONO_OUTPUTS + 6));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos8), module, Split::MONO_OUTPUTS + 7));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, Split::MONO_OUTPUTS + 8));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos2), module, Split::MONO_OUTPUTS + 9));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos3), module, Split::MONO_OUTPUTS + 10));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, Split::MONO_OUTPUTS + 11));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos5), module, Split::MONO_OUTPUTS + 12));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos6), module, Split::MONO_OUTPUTS + 13));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos7), module, Split::MONO_OUTPUTS + 14));
		addOutput(createOutputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos8), module, Split::MONO_OUTPUTS + 15));
	}
};


Model* modelSplit = createModel<Split, SplitWidget>("Split");
