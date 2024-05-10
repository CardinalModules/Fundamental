#include "components.hpp"


struct Quantizer : Module {
	enum ParamIds {
		OFFSET_PARAM, // TODO
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		PITCH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	bool enabledNotes[12];
	// Intervals [i / 24, (i+1) / 24) V mapping to the closest enabled note
	int ranges[24];
	bool playingNotes[12];

	Quantizer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(OFFSET_PARAM, -1.f, 1.f, 0.f, "Pre-offset", " semitones", 0.f, 12.f);
		configInput(PITCH_INPUT, "1V/octave pitch");
		configOutput(PITCH_OUTPUT, "Pitch");
		configBypass(PITCH_INPUT, PITCH_OUTPUT);

		onReset();
	}

	void onReset() override {
		for (int i = 0; i < 12; i++) {
			enabledNotes[i] = true;
		}
		updateRanges();
	}

	void onRandomize() override {
		for (int i = 0; i < 12; i++) {
			enabledNotes[i] = (random::uniform() < 0.5f);
		}
		updateRanges();
	}

	void process(const ProcessArgs& args) override {
		bool playingNotes[12] = {};
		int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);
		float offsetParam = params[OFFSET_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			float pitch = inputs[PITCH_INPUT].getVoltage(c);
			pitch += offsetParam;
			int range = std::floor(pitch * 24);
			int octave = eucDiv(range, 24);
			range -= octave * 24;
			int note = ranges[range] + octave * 12;
			playingNotes[eucMod(note, 12)] = true;
			pitch = float(note) / 12;
			outputs[PITCH_OUTPUT].setVoltage(pitch, c);
		}
		outputs[PITCH_OUTPUT].setChannels(channels);
		std::memcpy(this->playingNotes, playingNotes, sizeof(playingNotes));
	}

	void updateRanges() {
		// Check if no notes are enabled
		bool anyEnabled = false;
		for (int note = 0; note < 12; note++) {
			if (enabledNotes[note]) {
				anyEnabled = true;
				break;
			}
		}
		// Find closest notes for each range
		for (int i = 0; i < 24; i++) {
			int closestNote = 0;
			int closestDist = INT_MAX;
			for (int note = -12; note <= 24; note++) {
				int dist = std::abs((i + 1) / 2 - note);
				// Ignore enabled state if no notes are enabled
				if (anyEnabled && !enabledNotes[eucMod(note, 12)]) {
					continue;
				}
				if (dist < closestDist) {
					closestNote = note;
					closestDist = dist;
				}
				else {
					// If dist increases, we won't find a better one.
					break;
				}
			}
			ranges[i] = closestNote;
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_t* enabledNotesJ = json_array();
		for (int i = 0; i < 12; i++) {
			json_array_insert_new(enabledNotesJ, i, json_boolean(enabledNotes[i]));
		}
		json_object_set_new(rootJ, "enabledNotes", enabledNotesJ);

		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* enabledNotesJ = json_object_get(rootJ, "enabledNotes");
		if (enabledNotesJ) {
			for (int i = 0; i < 12; i++) {
				json_t* enabledNoteJ = json_array_get(enabledNotesJ, i);
				if (enabledNoteJ)
					enabledNotes[i] = json_boolean_value(enabledNoteJ);
			}
		}
		updateRanges();
	}
};


#ifndef HEADLESS
struct PianoNote : OpaqueWidget {
	enum State {
		kInit,
		kDisabled,
		kEnabledNotPlaying,
		kPlaying
	} state = kInit;

	int note;
	Quantizer* module;
	float opacities[2];
	NSVGshape* shapes[2] = {};
	FramebufferWidget* svgFb;

	PianoNote(Quantizer* const module, FramebufferWidget* const svgFb, std::shared_ptr<window::Svg>& pianoSvg, const int note) {
		this->note = note;
		this->module = module;
		this->svgFb = svgFb;

		NSVGimage* const handle = pianoSvg->handle;
		char shapeid[9] = {
			'k', 'e', 'y', '_',
			char('0' + ((note+1)/10)),
			char('0' + ((note+1) % 10)),
			'-', 'x', '\0'
		};

		// find shape one by one
		for (int i=0; i<2; ++i) {
			shapeid[7] = char('0' + i + 1);
			for (NSVGshape* shape = handle->shapes; shape != nullptr; shape = shape->next) {
				if (std::strcmp(shape->id, shapeid) == 0) {
					shapes[i] = shape;
					opacities[i] = shape->opacity;
					break;
				}
			}
			if (shapes[i] == nullptr) {
				break;
			}
		}
	}

	/* debug painting to ensure valid bounds
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
		nvgFillColor(args.vg, nvgRGB(0x12, 0xff, 0x12));
		nvgFill(args.vg);
	}
	*/

	void updateState() {
		for (int i=0; i<2; ++i) {
			if (shapes[i] == nullptr)
				break;
			shapes[i]->opacity = state == kDisabled ? 0.f : opacities[i] * (state == kPlaying ? 1.f : 0.5f);
		}
		if (svgFb != nullptr)
			svgFb->setDirty();
	}

	void onDragStart(const event::DragStart& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			module->enabledNotes[note] ^= true;
			module->updateRanges();
		}
		OpaqueWidget::onDragStart(e);
	}

	void onDragEnter(const event::DragEnter& e) override {
		if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if (PianoNote* const origin = dynamic_cast<PianoNote*>(e.origin)) {
				module->enabledNotes[note] = module->enabledNotes[origin->note];
				module->updateRanges();
			}
		}
		OpaqueWidget::onDragEnter(e);
	}

	void step() override {
		if (module == nullptr) {
			return;
		}

		const State newstate = module->playingNotes[note] ? kPlaying
							: module->enabledNotes[note] ? kEnabledNotPlaying : kDisabled;

		if (newstate != state) {
			state = newstate;
			updateState();
		}

		OpaqueWidget::step();
	}
};


struct WhitePianoNote : PianoNote {
	WhitePianoNote(Quantizer* const module, FramebufferWidget* const svgFb, std::shared_ptr<window::Svg>& pianoSvg, const int note)
		: PianoNote(module, svgFb, pianoSvg, note)
	{
		float y;
		switch (note) {
		case 11: y = 0.f; break;
		case  9: y = 24.f; break;
		case  7: y = 48.f; break;
		case  5: y = 72.f; break;
		case  4: y = 96.f; break;
		case  2: y = 120.f; break;
		case  0: y = 144.f; break;
		default: return;
		}
		box.pos = Vec(1.f, y);
		box.size = Vec(56.f, 23.f);
	}
};


struct BlackPianoNote : PianoNote {
	BlackPianoNote(Quantizer* const module, FramebufferWidget* const svgFb, std::shared_ptr<window::Svg>& pianoSvg, const int note)
		: PianoNote(module, svgFb, pianoSvg, note)
	{
		float y;
		switch (note) {
		case 10: y = 12.f; break;
		case  8: y = 40.f; break;
		case  6: y = 68.f; break;
		case  3: y = 110.f; break;
		case  1: y = 138.f; break;
		default: return;
		}
		box.pos = Vec(0.f, y);
		box.size = Vec(35.f, 16.f);
	}
};


struct PianoKeyboard : Widget {
	PianoKeyboard(Quantizer* const module, FramebufferWidget* const svgFb, std::shared_ptr<window::Svg>& pianoSvg) {
		// White notes
		static const std::vector<int> whiteNotes = {0, 2, 4, 5, 7, 9, 11};
		for (int note : whiteNotes) {
			WhitePianoNote* pianoNote = new WhitePianoNote(module, svgFb, pianoSvg, note);
			pianoNote->module = module;
			addChild(pianoNote);
		}
		// Black notes
		static const std::vector<int> blackNotes = {1, 3, 6, 8, 10};
		for (int note : blackNotes) {
			BlackPianoNote* const pianoNote = new BlackPianoNote(module, svgFb, pianoSvg, note);
			pianoNote->module = module;
			addChild(pianoNote);
		}
	}
};


struct QuantizerWidget : ModuleWidget {
	typedef FundamentalBlackKnob<30> Knob;

	static constexpr const int kWidth = 4;
	static constexpr const float kHorizontalCenter = kRACK_GRID_WIDTH * kWidth * 0.5f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 307.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 83.f - Knob::kHalfSize;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	std::shared_ptr<window::Svg> pianoSvg;

	QuantizerWidget(Quantizer* module) {
		setModule(module);
		app::SvgPanel* const panel = createPanel(asset::plugin(pluginInstance, "res/Quantizer.svg"));
		setPanel(panel);

		// make a copy so we can modify svg at runtime
		pianoSvg = std::make_shared<window::Svg>();
		pianoSvg->loadFile(asset::plugin(pluginInstance, "res/components/Quantizer-keyboard.svg"));

		addChild(createWidget<ThemedScrew>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(kRACK_GRID_WIDTH * 2, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<FundamentalPort>(Vec(kHorizontalCenter, kVerticalPos1), module, Quantizer::PITCH_INPUT));

		addParam(createParamCentered<Knob>(Vec(kHorizontalCenter, kVerticalPos2), module, Quantizer::OFFSET_PARAM));

		addOutput(createOutputCentered<FundamentalPort>(Vec(kHorizontalCenter, kVerticalPos3), module, Quantizer::PITCH_OUTPUT));

		PianoKeyboard* const pianoKeyboard = new PianoKeyboard(module, panel->fb, pianoSvg);
		pianoKeyboard->box.pos = Vec(2.f, kRACK_GRID_HEIGHT - 133.f - 167.f);
		addChild(pianoKeyboard);
	}

	void drawLayer(const DrawArgs& args, int layer) override
	{
		if (layer != 1)
			return Widget::drawLayer(args, layer);

		nvgSave(args.vg);
		nvgTranslate(args.vg, 2, 78);
		pianoSvg->draw(args.vg);
		nvgRestore(args.vg);
	}
};
#else
struct QuantizerWidget : ModuleWidget {
	QuantizerWidget(Quantizer* module) {
		setModule(module);
		addInput(createInputCentered<FundamentalPort>({}, module, Quantizer::PITCH_INPUT));
		addOutput(createOutputCentered<FundamentalPort>({}, module, Quantizer::PITCH_OUTPUT));
	}
};
#endif


Model* modelQuantizer = createModel<Quantizer, QuantizerWidget>("Quantizer");
