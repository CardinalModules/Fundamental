#include "components.hpp"


struct VCMixer : Module {
	enum ParamIds {
		MIX_LVL_PARAM,
		ENUMS(LVL_PARAMS, 4),
		NUM_PARAMS
	};
	enum InputIds {
		MIX_CV_INPUT,
		ENUMS(CH_INPUTS, 4),
		ENUMS(CV_INPUTS, 4),
		NUM_INPUTS
	};
	enum OutputIds {
		MIX_OUTPUT,
		ENUMS(CH_OUTPUTS, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LVL_LIGHTS, 4),
		NUM_LIGHTS
	};

	VCMixer() {
		config(0, 0, 0, 0);
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// x^1 scaling up to 6 dB
		configParam(MIX_LVL_PARAM, 0.0, 2.0, 1.0, "Mix level", " dB", -10, 20);
		// x^2 scaling up to 6 dB
		configParam(LVL_PARAMS + 0, 0.0, M_SQRT2, 1.0, "Channel 1 level", " dB", -10, 40);
		configParam(LVL_PARAMS + 1, 0.0, M_SQRT2, 1.0, "Channel 2 level", " dB", -10, 40);
		configParam(LVL_PARAMS + 2, 0.0, M_SQRT2, 1.0, "Channel 3 level", " dB", -10, 40);
		configParam(LVL_PARAMS + 3, 0.0, M_SQRT2, 1.0, "Channel 4 level", " dB", -10, 40);
		configInput(MIX_CV_INPUT, "Mix CV");
		for (int i = 0; i < 4; i++)
			configInput(CH_INPUTS + i, string::f("Channel %d", i + 1));
		for (int i = 0; i < 4; i++)
			configInput(CV_INPUTS + i, string::f("Channel %d CV", i + 1));
		configOutput(MIX_OUTPUT, "Mix");
		for (int i = 0; i < 4; i++)
			configOutput(CH_OUTPUTS + i, string::f("Channel %d", i + 1));
	}

	void process(const ProcessArgs& args) override {
		// Get number of poly channels for mix output
		int mixChannels = 1;
		for (int i = 0; i < 4; i++) {
			mixChannels = std::max(mixChannels, inputs[CH_INPUTS + i].getChannels());
		}
		float mix[16] = {};

		// Channel strips
		for (int i = 0; i < 4; i++) {
			int channels = 1;
			float in[16] = {};
			float sum = 0.f;

			if (inputs[CH_INPUTS + i].isConnected()) {
				channels = inputs[CH_INPUTS + i].getChannels();

				// Get input
				inputs[CH_INPUTS + i].readVoltages(in);

				// Apply fader gain
				float gain = std::pow(params[LVL_PARAMS + i].getValue(), 2.f);
				for (int c = 0; c < channels; c++) {
					in[c] *= gain;
				}

				// Apply CV gain
				if (inputs[CV_INPUTS + i].isConnected()) {
					for (int c = 0; c < channels; c++) {
						float cv = clamp(inputs[CV_INPUTS + i].getPolyVoltage(c) / 10.f, 0.f, 1.f);
						in[c] *= cv;
					}
				}

				// Add to mix
				for (int c = 0; c < channels; c++) {
					mix[c] += in[c];
				}

				// Sum channel for VU meter
				for (int c = 0; c < channels; c++) {
					sum += in[c];
				}
			}

			// Set channel output
			if (outputs[CH_OUTPUTS + i].isConnected()) {
				outputs[CH_OUTPUTS + i].setChannels(channels);
				outputs[CH_OUTPUTS + i].writeVoltages(in);
			}
		}

		// Mix output
		if (outputs[MIX_OUTPUT].isConnected()) {
			// Apply mix knob gain
			float gain = params[MIX_LVL_PARAM].getValue();
			for (int c = 0; c < mixChannels; c++) {
				mix[c] *= gain;
			}

			// Apply mix CV gain
			if (inputs[MIX_CV_INPUT].isConnected()) {
				for (int c = 0; c < mixChannels; c++) {
					float cv = clamp(inputs[MIX_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
					mix[c] *= cv;
				}
			}

			// Set mix output
			outputs[MIX_OUTPUT].setChannels(mixChannels);
			outputs[MIX_OUTPUT].writeVoltages(mix);
		}
	}
};


struct VCMixerWidget : ModuleWidget {
	typedef CardinalBlackKnob<40> BigKnob;
	typedef CardinalBlackKnob<27> MediumKnob;
	typedef CardinalBlackKnob<18> SmallKnob;

	static constexpr const int kWidth = 9;
	static constexpr const float kBorderPadding = 12.5f + kRACK_JACK_HALF_SIZE;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kHorizontalCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;

	static constexpr const float kHorizontalAdvance = 12.5f + kRACK_JACK_HALF_SIZE;
	static constexpr const float kHorizontalPos1 = kHorizontalAdvance + kUsableWidth * 0.f / 3.f;
	static constexpr const float kHorizontalPos2 = kHorizontalAdvance + kUsableWidth * 1.f / 3.f;
	static constexpr const float kHorizontalPos3 = kHorizontalAdvance + kUsableWidth * 2.f / 3.f;
	static constexpr const float kHorizontalPos4 = kHorizontalAdvance + kUsableWidth * 3.f / 3.f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 309.5f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 245.f - MediumKnob::kHalfSize;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 214.f - SmallKnob::kHalfSize;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 186.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 133.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 64.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	VCMixerWidget(VCMixer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VCMixer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos1, kVerticalPos1), module, VCMixer::CH_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos2, kVerticalPos1), module, VCMixer::CH_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos3, kVerticalPos1), module, VCMixer::CH_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos4, kVerticalPos1), module, VCMixer::CH_INPUTS + 3));

		addParam(createParamCentered<MediumKnob>(Vec(kHorizontalPos1, kVerticalPos2), module, VCMixer::LVL_PARAMS + 0));
		addParam(createParamCentered<MediumKnob>(Vec(kHorizontalPos2, kVerticalPos2), module, VCMixer::LVL_PARAMS + 1));
		addParam(createParamCentered<MediumKnob>(Vec(kHorizontalPos3, kVerticalPos2), module, VCMixer::LVL_PARAMS + 2));
		addParam(createParamCentered<MediumKnob>(Vec(kHorizontalPos4, kVerticalPos2), module, VCMixer::LVL_PARAMS + 3));

		// FIXME missing cv level params @ kVerticalPos3

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos1, kVerticalPos4), module, VCMixer::CV_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos2, kVerticalPos4), module, VCMixer::CV_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos3, kVerticalPos4), module, VCMixer::CV_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalPos4, kVerticalPos4), module, VCMixer::CV_INPUTS + 3));

		addInput(createInputCentered<CardinalPort>(Vec(14.925f + kRACK_JACK_HALF_SIZE, kVerticalPos5), module, VCMixer::MIX_CV_INPUT));
		// FIXME missing cv mix param @ 52.74 + SmallKnob::kHalfSize / kVerticalPos5
		addParam(createParamCentered<BigKnob>(Vec(86.f + BigKnob::kHalfSize, kVerticalPos5), module, VCMixer::MIX_LVL_PARAM));

		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos6), module, VCMixer::MIX_OUTPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos1, kVerticalPos7), module, VCMixer::CH_OUTPUTS + 0));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos2, kVerticalPos7), module, VCMixer::CH_OUTPUTS + 1));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos3, kVerticalPos7), module, VCMixer::CH_OUTPUTS + 2));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalPos4, kVerticalPos7), module, VCMixer::CH_OUTPUTS + 3));
	}
};


Model* modelVCMixer = createModel<VCMixer, VCMixerWidget>("VCMixer");
