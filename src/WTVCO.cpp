#include "components.hpp"
#include "Wavetable.hpp"


using simd::float_4;


struct WTVCO : Module {
	enum ParamIds {
		MODE_PARAM, // removed
		SOFT_PARAM,
		FREQ_PARAM,
		POS_PARAM,
		FM_PARAM,
		// added in 2.0
		POS_CV_PARAM,
		LINEAR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		FM_INPUT,
		SYNC_INPUT,
		POS_INPUT,
		// added in 2.0
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		WAVE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PHASE_LIGHT, 3),
		SOFT_LIGHT,
		LINEAR_LIGHT,
		NUM_LIGHTS
	};

	Wavetable wavetable;
	float_4 phases[4] = {};
	float lastPos = 0.f;
	dsp::MinBlepGenerator<16, 16, float_4> syncMinBleps[4];
	float_4 lastSyncValues[4] = {};
	float_4 syncDirections[4] = {};

	dsp::ClockDivider lightDivider;
	dsp::BooleanTrigger softTrigger;
	dsp::BooleanTrigger linearTrigger;

	WTVCO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configSwitch(SOFT_PARAM, 0.f, 1.f, 0.f, "Sync", {"Hard", "Soft"});
		configSwitch(LINEAR_PARAM, 0.f, 1.f, 0.f, "FM mode", {"1V/octave", "Through-zero linear"});

		configParam(FREQ_PARAM, -75.f, 75.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(POS_PARAM, 0.f, 1.f, 0.f, "Wavetable position", "%", 0.f, 100.f);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		getParamQuantity(FM_PARAM)->randomizeEnabled = false;
		configParam(POS_CV_PARAM, -1.f, 1.f, 0.f, "Wavetable position CV", "%", 0.f, 100.f);
		getParamQuantity(POS_CV_PARAM)->randomizeEnabled = false;

		configInput(FM_INPUT, "Frequency modulation");
		configInput(SYNC_INPUT, "Sync");
		configInput(POS_INPUT, "Wavetable position");
		configInput(PITCH_INPUT, "1V/octave pitch");

		configOutput(WAVE_OUTPUT, "Wavetable");

		configLight(PHASE_LIGHT, "Phase");

		wavetable.setQuality(8);

		lightDivider.setDivision(16);

		onReset();
	}

	void onReset() override {
		wavetable.reset();
		for (int i = 0; i < 4; i++) {
			syncDirections[i] = 1.f;
		}
	}

	void onAdd(const AddEvent& e) override {
		std::string path = system::join(getPatchStorageDirectory(), "wavetable.wav");
		// Silently fails
		wavetable.load(path);
	}

	void onSave(const SaveEvent& e) override {
		if (!wavetable.samples.empty()) {
			std::string path = system::join(createPatchStorageDirectory(), "wavetable.wav");
			wavetable.save(path);
		}
	}

	void clearOutput() {
		outputs[WAVE_OUTPUT].setVoltage(0.f);
		outputs[WAVE_OUTPUT].setChannels(1);
	}

	float getWave(float index, float pos, float octave) {
		// Get wave indexes
		float indexF = index - std::trunc(index);
		size_t index0 = std::trunc(index);
		size_t index1 = (index0 + 1) % (wavetable.waveLen * wavetable.quality);
		// Get position indexes
		float posF = pos - std::trunc(pos);
		size_t pos0 = std::trunc(pos);
		size_t pos1 = pos0 + 1;
		// Get octave index
		// float octaveF = octave - std::trunc(octave);
		size_t octave0 = std::trunc(octave);
		octave0 = std::min(octave0, wavetable.octaves - 1);
		// size_t octave1 = octave0 + 1;

		// Linearly interpolate wave index
		float out = crossfade(wavetable.interpolatedAt(octave0, pos0, index0), wavetable.interpolatedAt(octave0, pos0, index1), indexF);
		// Interpolate octave
		// if (octaveF > 0.f && octave1 < wavetable.octaves) {
		// 	float out1 = crossfade(wavetable.interpolatedAt(octave1, pos0, index0), wavetable.interpolatedAt(octave1, pos0, index1), indexF);
		// 	out = crossfade(out, out1, octaveF);
		// }
		// Linearly interpolate position if needed
		if (posF > 0.f) {
			float out1 = crossfade(wavetable.interpolatedAt(octave0, pos1, index0), wavetable.interpolatedAt(octave0, pos1, index1), indexF);
			// Interpolate octave
			// if (octaveF > 0.f && octave1 < wavetable.octaves) {
			// 	float out2 = crossfade(wavetable.interpolatedAt(octave1, pos1, index0), wavetable.interpolatedAt(octave1, pos1, index1), indexF);
			// 	out1 = crossfade(out1, out2, octaveF);
			// }
			out = crossfade(out, out1, posF);
		}
		return out;
	}

	void process(const ProcessArgs& args) override {
		float freqParam = params[FREQ_PARAM].getValue() / 12.f;
		float fmParam = params[FM_PARAM].getValue();
		float posParam = params[POS_PARAM].getValue();
		float posCvParam = params[POS_CV_PARAM].getValue();
		bool soft = params[SOFT_PARAM].getValue() > 0.f;
		bool linear = params[LINEAR_PARAM].getValue() > 0.f;
		bool syncEnabled = inputs[SYNC_INPUT].isConnected();

		int channels = std::max({1, inputs[PITCH_INPUT].getChannels(), inputs[FM_INPUT].getChannels()});

		int waveCount = wavetable.getWaveCount();
		if (!wavetable.loading && wavetable.waveLen >= 2 && waveCount >= 1) {
			// Iterate channels
			for (int c = 0; c < channels; c += 4) {
				// Calculate frequency in Hz
				float_4 pitch = freqParam + inputs[PITCH_INPUT].getPolyVoltageSimd<float_4>(c);
				float_4 freq;
				if (!linear) {
					pitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c) * fmParam;
					freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
				}
				else {
					freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
					freq += dsp::FREQ_C4 * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c) * fmParam;
				}

				// Limit to Nyquist frequency
				freq = simd::fmin(freq, args.sampleRate / 2);
				float_4 octave = simd::log2(args.sampleRate / 2 / freq);

				// Accumulate phase
				float_4 phase = phases[c / 4];
				if (!soft) {
					syncDirections[c / 4] = 1.f;
				}
				// Delta phase can be negative
				float_4 deltaPhase = freq * args.sampleTime * syncDirections[c / 4];
				phase += deltaPhase;
				// Wrap phase
				phase -= simd::floor(phase);
				phases[c / 4] = phase;
				float_4 index = phase * wavetable.waveLen * wavetable.quality;

				// Get wavetable position, scaled from 0 to (waveCount - 1)
				float_4 pos = posParam + inputs[POS_INPUT].getPolyVoltageSimd<float_4>(c) * posCvParam / 10.f;
				pos = simd::clamp(pos);
				pos *= (waveCount - 1);

				if (c == 0)
					lastPos = pos[0];

				// Get wave output serially
				int ccs = std::min(4, channels - c);
				float_4 out = 0.f;
				for (int cc = 0; cc < ccs; cc++) {
					out[cc] = getWave(index[cc], pos[cc], octave[cc]);
				}

				// Sync
				if (syncEnabled) {
					float_4 syncValue = inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c);
					float_4 deltaSync = syncValue - lastSyncValues[c / 4];
					float_4 syncCrossing = -lastSyncValues[c / 4] / deltaSync;
					lastSyncValues[c / 4] = syncValue;
					float_4 sync = (0.f < syncCrossing) & (syncCrossing <= 1.f) & (syncValue >= 0.f);
					int syncMask = simd::movemask(sync);
					if (syncMask) {
						if (soft) {
							syncDirections[c / 4] = simd::ifelse(sync, -syncDirections[c / 4], syncDirections[c / 4]);
						}
						else {
							phases[c / 4] = simd::ifelse(sync, (1.f - syncCrossing) * deltaPhase, phases[c / 4]);
							// Insert minBLEP for sync serially
							for (int cc = 0; cc < ccs; cc++) {
								if (syncMask & (1 << cc)) {
									float_4 mask = simd::movemaskInverse<float_4>(1 << cc);
									float p = syncCrossing[cc] - 1.f;
									float index = phases[c / 4][cc] * wavetable.waveLen * wavetable.quality;
									float out1 = getWave(index, pos[cc], octave[cc]);
									float_4 x = mask & (out1 - out[cc]);
									syncMinBleps[c / 4].insertDiscontinuity(p, x);
								}
							}
						}
					}
				}
				out += syncMinBleps[c / 4].process();

				outputs[WAVE_OUTPUT].setVoltageSimd(out * 5.f, c);
			}
		}
		else {
			// Wavetable is invalid, so set 0V
			for (int c = 0; c < channels; c += 4) {
				outputs[WAVE_OUTPUT].setVoltageSimd(float_4(0.f), c);
			}
		}

		outputs[WAVE_OUTPUT].setChannels(channels);

		// Light
		if (lightDivider.process()) {
			lights[LINEAR_LIGHT].setBrightness(linear);
			lights[SOFT_LIGHT].setBrightness(soft);
		}
	}

	void paramsFromJson(json_t* rootJ) override {
		// In <2.0, there were no attenuverters, so set them to 1.0 in case they are not overwritten.
		params[POS_CV_PARAM].setValue(1.f);
		Module::paramsFromJson(rootJ);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// Merge wavetable
		json_t* wavetableJ = wavetable.toJson();
		json_object_update(rootJ, wavetableJ);
		json_decref(wavetableJ);
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		// wavetable
		wavetable.fromJson(rootJ);
	}
};


struct WTVCOWidget : ModuleWidget {
	typedef CardinalBlackKnob<40> BigKnob;
	typedef CardinalBlackKnob<18> SmallKnob;

	static constexpr const int kWidth = 7;
	static constexpr const float kBorderPadding = 5.f;
	static constexpr const float kUsableWidth = kRACK_GRID_WIDTH * kWidth - kBorderPadding * 2.f;

	static constexpr const float kPosLeft = kBorderPadding + kUsableWidth * 0.25f;
	static constexpr const float kPosCenter = kBorderPadding + kUsableWidth * 0.5f;
	static constexpr const float kPosRight = kBorderPadding + kUsableWidth * 0.75f;

	static constexpr const float kVerticalPos1 = kRACK_GRID_HEIGHT - 306.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos2 = kRACK_GRID_HEIGHT - 226.f - BigKnob::kHalfSize;
	static constexpr const float kVerticalPos3 = kRACK_GRID_HEIGHT - 196.f - SmallKnob::kHalfSize;
	static constexpr const float kVerticalPos4 = kRACK_GRID_HEIGHT - 156.f - kRACK_JACK_HALF_SIZE;
	static constexpr const float kVerticalPos5 = kRACK_GRID_HEIGHT - 101.f;
	static constexpr const float kVerticalPos6 = kRACK_GRID_HEIGHT - 26.f - kRACK_JACK_HALF_SIZE;

	WTVCOWidget(WTVCO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/WTVCO.svg")));

		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * kRACK_GRID_WIDTH, kRACK_GRID_HEIGHT - kRACK_GRID_WIDTH)));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos1), module, WTVCO::PITCH_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos1), module, WTVCO::SYNC_INPUT));

		addParam(createParamCentered<BigKnob>(Vec(kPosLeft, kVerticalPos2), module, WTVCO::FREQ_PARAM));
		addParam(createParamCentered<BigKnob>(Vec(kPosRight, kVerticalPos2), module, WTVCO::POS_PARAM));

		addParam(createParamCentered<SmallKnob>(Vec(kPosLeft, kVerticalPos3), module, WTVCO::FM_PARAM));
		addParam(createParamCentered<SmallKnob>(Vec(kPosRight, kVerticalPos3), module, WTVCO::POS_CV_PARAM));

		addInput(createInputCentered<CardinalPort>(Vec(kPosLeft, kVerticalPos4), module, WTVCO::FM_INPUT));
		addInput(createInputCentered<CardinalPort>(Vec(kPosRight, kVerticalPos4), module, WTVCO::POS_INPUT));

		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosLeft, kVerticalPos5), module, WTVCO::LINEAR_PARAM, WTVCO::LINEAR_LIGHT));
		addParam(createLightParamCentered<CardinalLightLatch>(Vec(kPosRight, kVerticalPos5), module, WTVCO::SOFT_PARAM, WTVCO::SOFT_LIGHT));

		addOutput(createOutputCentered<CardinalPort>(Vec(kPosCenter, kVerticalPos6), module, WTVCO::WAVE_OUTPUT));

		/* TODO
		WTDisplay<WTVCO>* display = createWidget<WTDisplay<WTVCO>>(Vec(0.004, 13.04));
		display->box.size = Vec(35.56, 29.224);
		display->module = module;
		addChild(display);
		*/
	}

	void appendContextMenu(Menu* menu) override {
		WTVCO* module = dynamic_cast<WTVCO*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		module->wavetable.appendContextMenu(menu);
	}
};


Model* modelVCO2 = createModel<WTVCO, WTVCOWidget>("VCO2");
