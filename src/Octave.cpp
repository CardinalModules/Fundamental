#include "components.hpp"


struct Octave : Module {
	enum ParamIds {
		OCTAVE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		OCTAVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	int lastOctave = 0;

	Octave() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(OCTAVE_PARAM, -4.f, 4.f, 0.f, "Shift", " oct");
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configInput(PITCH_INPUT, "1V/octave pitch");
		configInput(OCTAVE_INPUT, "Octave shift CV");
		configOutput(PITCH_OUTPUT, "Pitch");
		configBypass(PITCH_INPUT, PITCH_OUTPUT);
	}

	void process(const ProcessArgs& args) override {
		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);
		int octaveParam = std::round(params[OCTAVE_PARAM].getValue());

		for (int c = 0; c < channels; c++) {
			int octave = octaveParam + std::round(inputs[OCTAVE_INPUT].getPolyVoltage(c));
			float pitch = inputs[PITCH_INPUT].getVoltage(c);
			pitch += octave;
			outputs[PITCH_OUTPUT].setVoltage(pitch, c);
			if (c == 0)
				lastOctave = octave;
		}
		outputs[PITCH_OUTPUT].setChannels(channels);
	}

	void dataFromJson(json_t* rootJ) override {
		// In Fundamental 1.1.1 and earlier, the octave param was internal data.
		json_t* octaveJ = json_object_get(rootJ, "octave");
		if (octaveJ) {
			params[OCTAVE_PARAM].setValue(json_integer_value(octaveJ));
		}
	}
};


struct OctaveButton : Widget {
	int octave;

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer != 1)
			return;

		Vec c = box.size.div(2);

		int activeOctave = 0;
		int lastOctave = 0;
		ParamWidget* paramWidget = getAncestorOfType<ParamWidget>();
		assert(paramWidget);
		engine::ParamQuantity* pq = paramWidget->getParamQuantity();
		if (pq) {
			activeOctave = std::round(pq->getValue());
			Octave* module = dynamic_cast<Octave*>(pq->module);
			if (module)
				lastOctave = module->lastOctave;
		}

		if (activeOctave == octave) {
			// Enabled
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, 4.0 / 2);
			if (octave == 0)
				nvgFillColor(args.vg, color::alpha(color::WHITE, 0.33));
			else
				nvgFillColor(args.vg, SCHEME_YELLOW);
			nvgFill(args.vg);
		}
		else if (lastOctave == octave) {
			// Disabled but enabled by CV
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, 4.0 / 2);
			if (octave == 0)
				nvgFillColor(args.vg, color::alpha(color::WHITE, 0.5 * 0.33));
			else
				nvgFillColor(args.vg, color::alpha(SCHEME_YELLOW, 0.5));
			nvgFill(args.vg);
		}
		else {
			// Disabled
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, 4.0 / 2);
			nvgFillColor(args.vg, color::alpha(color::WHITE, 0.33));
			nvgFill(args.vg);

			nvgBeginPath(args.vg);
			nvgCircle(args.vg, c.x, c.y, 3.0 / 2);
			nvgFillColor(args.vg, nvgRGB(0x12, 0x12, 0x12));
			nvgFill(args.vg);

			if (octave == 0) {
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, c.x, c.y, 1.0 / 2);
				nvgFillColor(args.vg, color::alpha(color::WHITE, 0.33));
				nvgFill(args.vg);
			}
		}
	}

	void onDragHover(const event::DragHover& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
		}
		Widget::onDragHover(e);
	}

	void onDragEnter(const event::DragEnter& e) override;
};


struct OctaveParam : ParamWidget {
	OctaveParam() {
		box.size = Vec(15.263, 55.88);
		const int octaves = 9;
		const float margin = 2.0;
		float height = box.size.y - 2 * margin;
		for (int i = 0; i < octaves; i++) {
			OctaveButton* octaveButton = new OctaveButton();
			octaveButton->box.pos = Vec(0, height / octaves * i + margin);
			octaveButton->box.size = Vec(box.size.x, height / octaves);
			octaveButton->octave = 4 - i;
			addChild(octaveButton);
		}
	}
};


inline void OctaveButton::onDragEnter(const event::DragEnter& e) {
	if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
		OctaveParam* origin = dynamic_cast<OctaveParam*>(e.origin);
		if (origin) {
			ParamWidget* paramWidget = getAncestorOfType<ParamWidget>();
			assert(paramWidget);
			engine::ParamQuantity* pq = paramWidget->getParamQuantity();
			if (pq) {
				pq->setValue(octave);
			}
		}
	}
	Widget::onDragEnter(e);
}


struct OctaveDisplay : LedDisplay {
	void setModule(Octave* module) {
		addChild(createParam<OctaveParam>(Vec(0.0, 0.0), module, Octave::OCTAVE_PARAM));
	}
};


struct OctaveWidget : ModuleWidget {
	static constexpr const int kWidth = 3;
	static constexpr const float kHorizontalCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 308.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 268.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	OctaveWidget(Octave* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Octave.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos1), module, Octave::OCTAVE_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos2), module, Octave::PITCH_INPUT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kHorizontalCenter, kVerticalPos3), module, Octave::PITCH_OUTPUT));

		/* TODO
		OctaveDisplay* display = createWidget<OctaveDisplay>(Vec(0.0, 13.039));
		display->box.size = Vec(15.263, 55.88);
		display->setModule(module);
		addChild(display);
		*/
	}
};


Model* modelOctave = createModel<Octave, OctaveWidget>("Octave");
