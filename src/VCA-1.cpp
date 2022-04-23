#include "components.hpp"


struct VCA_1 : Module {
	enum ParamIds {
		LEVEL_PARAM,
		EXP_PARAM, // removed from panel in 2.0, still in context menu
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		IN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int lastChannels = 1;
	float lastGains[16] = {};

	VCA_1() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(LEVEL_PARAM, 0.0, 1.0, 1.0, "Level", "%", 0, 100);
		configSwitch(EXP_PARAM, 0.0, 1.0, 1.0, "Response mode", {"Exponential", "Linear"});
		configInput(CV_INPUT, "CV");
		configInput(IN_INPUT, "Channel");
		configOutput(OUT_OUTPUT, "Channel");
		configBypass(IN_INPUT, OUT_OUTPUT);
	}

	void process(const ProcessArgs& args) override {
		int channels = std::max({1, inputs[IN_INPUT].getChannels(), inputs[CV_INPUT].getChannels()});
		float level = params[LEVEL_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			// Get input
			float in = inputs[IN_INPUT].getPolyVoltage(c);

			// Get gain
			float gain = level;
			if (inputs[CV_INPUT].isConnected()) {
				float cv = clamp(inputs[CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
				if (int(params[EXP_PARAM].getValue()) == 0)
					cv = std::pow(cv, 4.f);
				gain *= cv;
			}

			// Apply gain
			in *= gain;
			lastGains[c] = gain;

			// Set output
			outputs[OUT_OUTPUT].setVoltage(in, c);
		}

		outputs[OUT_OUTPUT].setChannels(channels);
		lastChannels = channels;
	}
};


struct VCA_1VUKnob : SliderKnob {
	NVGcolor bgColor = nvgRGB(0x12, 0x12, 0x12);

	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
		nvgFillColor(args.vg, bgColor);
		nvgFill(args.vg);
		nvgStrokeColor(args.vg, nvgRGB(0x4a, 0x44, 0x44));
		nvgStrokeWidth(args.vg, 2.f);
		nvgStroke(args.vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1) {
			return SliderKnob::drawLayer(args, layer);
		}

		Rect r = box.zeroPos().shrink(Vec(2.f, 2.f));
		VCA_1* module = dynamic_cast<VCA_1*>(this->module);

		int channels = module ? module->lastChannels : 1;
		engine::ParamQuantity* pq = getParamQuantity();
		float value = pq ? pq->getValue() : 1.f;

		// Segment value
		if (value >= 0.005f) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg,
			        r.pos.x,
			        r.pos.y + r.size.y * (1 - value),
			        r.size.x,
			        r.size.y * value);
			nvgFillColor(args.vg, color::mult(color::WHITE, 0.25));
			nvgFill(args.vg);
		}

		// Segment gain
		for (int c = 0; c < channels; c++) {
			float gain = module ? module->lastGains[c] : 1.f;
			if (gain >= 0.005f) {
				nvgBeginPath(args.vg);
				nvgRect(args.vg,
				        r.pos.x + r.size.x * c / channels,
				        r.pos.y + r.size.y * (1 - gain),
				        r.size.x / channels,
				        r.size.y * gain);
				nvgFillColor(args.vg, nvgRGBf(0.76f, 0.11f, 0.22f));
				nvgFill(args.vg);
			}
		}

		// Invisible separators
		const int segs = 25;
		nvgFillColor(args.vg, bgColor);
		for (int i = 1; i < segs; i++) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg,
			        r.pos.x - 1.0,
			        r.pos.y + r.size.y * i / segs,
			        r.size.x + 2.0,
			        1.0);
			nvgFill(args.vg);
		}
	}
};


struct VCA_1Widget : ModuleWidget {
	static constexpr const int kWidth = 3;
	static constexpr const float kHorizontalCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;

	VCA_1Widget(VCA_1* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/VCA-1.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kRACK_GRID_HEIGHT - 307.f - 11.f), module, VCA_1::IN_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kRACK_GRID_HEIGHT - 80.f - 11.f), module, VCA_1::CV_INPUT));
		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalCenter, kRACK_GRID_HEIGHT - 25.f - 11.f), module, VCA_1::OUT_OUTPUT));

		VCA_1VUKnob* knob = createParam<VCA_1VUKnob>(Vec(6.5f, kRACK_GRID_HEIGHT - 120.f - 176.f), module, VCA_1::LEVEL_PARAM);
		knob->box.size = Vec(32.f, 176.f);
		addChild(knob);
	}

	void appendContextMenu(Menu* menu) override {
		VCA_1* module = dynamic_cast<VCA_1*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createBoolMenuItem("Exponential response", "",
			[=]() {return module->params[VCA_1::EXP_PARAM].getValue() == 0.f;},
			[=](bool value) {module->params[VCA_1::EXP_PARAM].setValue(!value);}
		));
	}
};


Model* modelVCA_1 = createModel<VCA_1, VCA_1Widget>("VCA-1");
