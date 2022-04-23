#include "components.hpp"


using simd::float_4;


struct Mixer : Module {
	enum ParamId {
		LEVEL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(IN_INPUTS, 6),
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	bool invert = false;
	bool average = false;

	Mixer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LEVEL_PARAM, 0.f, 1.f, 1.f, "Level", "%", 0, 100);
		for (int i = 0; i < 6; i++)
			configInput(IN_INPUTS + i, string::f("Channel %d", i + 1));
		configOutput(OUT_OUTPUT, "Mix");
	}

	void process(const ProcessArgs& args) override {
		// Get number of channels and number of connected inputs
		int channels = 1;
		int connected = 0;
		for (int i = 0; i < 6; i++) {
			channels = std::max(channels, inputs[IN_INPUTS + i].getChannels());
			if (inputs[IN_INPUTS + i].isConnected())
				connected++;
		}

		float gain = params[LEVEL_PARAM].getValue();
		// Invert
		if (invert) {
			gain *= -1.f;
		}
		// Average
		if (average) {
			gain /= std::max(1, connected);
		}

		// Iterate polyphonic channels
		for (int c = 0; c < channels; c += 4) {
			float_4 out = 0.f;
			// Mix input
			for (int i = 0; i < 6; i++) {
				out += inputs[IN_INPUTS + i].getVoltageSimd<float_4>(c);
			}

			// Apply gain
			out *= gain;

			// Set output
			outputs[OUT_OUTPUT].setVoltageSimd(out, c);
		}

		outputs[OUT_OUTPUT].setChannels(channels);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// average
		json_object_set_new(rootJ, "average", json_boolean(average));
		// invert
		json_object_set_new(rootJ, "invert", json_boolean(invert));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		// average
		json_t* averageJ = json_object_get(rootJ, "average");
		if (averageJ)
			average = json_boolean_value(averageJ);
		// invert
		json_t* invertJ = json_object_get(rootJ, "invert");
		if (invertJ)
			invert = json_boolean_value(invertJ);
	}
};


struct MixerWidget : ModuleWidget {
	typedef CardinalBlackKnob<30> Knob;

	static constexpr const int kWidth = 3;
	static constexpr const float kHorizontalCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 308.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 276.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 244.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 212.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 180.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 148.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos7 = kRACK_GRID_HEIGHT - 75.f - Knob::kHalfSize;
	static constexpr const float kVerticalPos8 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	MixerWidget(Mixer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Mixer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos1), module, Mixer::IN_INPUTS + 0));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos2), module, Mixer::IN_INPUTS + 1));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos3), module, Mixer::IN_INPUTS + 2));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos4), module, Mixer::IN_INPUTS + 3));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos5), module, Mixer::IN_INPUTS + 4));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos6), module, Mixer::IN_INPUTS + 5));

		addParam(createParamCentered<Knob>(Vec(kHorizontalCenter, kVerticalPos7), module, Mixer::LEVEL_PARAM));

		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos8), module, Mixer::OUT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Mixer* module = dynamic_cast<Mixer*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createBoolPtrMenuItem("Invert output", "", &module->invert));
		menu->addChild(createBoolPtrMenuItem("Average voltages", "", &module->average));
	}
};


Model* modelMixer = createModel<Mixer, MixerWidget>("Mixer");
