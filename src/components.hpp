/*
 * DISTRHO Fundamental components
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#pragma once

#include "plugin.hpp"

static constexpr const float kRACK_GRID_WIDTH = 15.f;
static constexpr const float kRACK_GRID_HEIGHT = 380.f;

static const constexpr float kRACK_JACK_SIZE = 32.f;
static const constexpr float kRACK_JACK_HALF_SIZE = kRACK_JACK_SIZE * 0.5f;

template<int size>
struct CardinalBlackKnob : RoundKnob {
	static constexpr const float kSize = size;
	static constexpr const float kHalfSize = size * 0.5f;
	float scale;

	CardinalBlackKnob() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/knob-marker.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/knob.svg")));

		scale = size / sw->box.size.x;
		box.size = Vec(size, size);
		bg->box.size = Vec(size, size);
	}

	void draw(const DrawArgs& args) override{
		nvgSave(args.vg);
		nvgScale(args.vg, scale, scale);
		RoundKnob::draw(args);
		nvgRestore(args.vg);
	}
};

typedef CardinalBlackKnob<42> CardinalBigBlackKnob;
typedef CardinalBlackKnob<20> CardinalSmallBlackKnob;
