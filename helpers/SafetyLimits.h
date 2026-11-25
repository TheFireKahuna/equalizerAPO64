/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2026

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <cmath>

namespace SafetyLimits
{
	// Maximum number of lines/filters read from config.txt and allowed in the Editor
	constexpr int MAX_FILTER_COUNT = 20;

	// Allowed gain range (dB) for Preamp, BiQuad, and GraphicEQ nodes
	constexpr double MIN_GAIN_DB = -10.0;
	constexpr double MAX_GAIN_DB = 10.0;

	// Allowed frequency range (Hz) for BiQuad and GraphicEQ nodes
	constexpr double MIN_FREQ_HZ = 10.0;
	constexpr double MAX_FREQ_HZ = 20000.0;

	// Allowed Q range for BiQuad filters
	constexpr double MIN_Q = 0.33;
	constexpr double MAX_Q = 33.33;

	// Equivalent bandwidth range (octaves) corresponding to Q in [0.33, 33.33]:
	// BW = (2 / ln(2)) * asinh(1 / (2 * Q)) => [0.043285, 3.475489]
	constexpr double MIN_BW_OCT_GUI = 0.0433;
	constexpr double MAX_BW_OCT_GUI = 3.4754;
	constexpr double MIN_BW_OCT = 0.043;
	constexpr double MAX_BW_OCT = 3.48;

	// Allowed shelf slope range (dB/Oct) for Low-shelf and High-shelf filters
	constexpr double MIN_SLOPE_DB = 1.0;
	constexpr double MAX_SLOPE_DB = 12.0;

	// Maximum number of nodes in a single GraphicEQ filter
	constexpr int MAX_GRAPHIC_EQ_NODES = 256;

	// Maximum total peak gain (dB) of the combined filter chain before triggering full Bypass
	constexpr double MAX_PEAK_GAIN_DB = 5.0;

	// Instantaneous zero-latency output sample clamp (+/- 2.0 = +6.02 dBFS)
	// Protects against time-domain phase-dispersion overshoots on square waves (e.g. All-pass / High-pass)
	constexpr double HARD_CLIP_AMPLITUDE = 2.0;

	// Small tolerance for floating-point boundary comparisons during text parsing
	constexpr double EPSILON = 1e-7;

	inline bool isGainValid(double dbGain)
	{
		return std::isfinite(dbGain)
			&& dbGain >= (MIN_GAIN_DB - EPSILON)
			&& dbGain <= (MAX_GAIN_DB + EPSILON);
	}

	inline bool isFreqValid(double freqHz)
	{
		return std::isfinite(freqHz)
			&& freqHz >= (MIN_FREQ_HZ - EPSILON)
			&& freqHz <= (MAX_FREQ_HZ + EPSILON);
	}

	inline bool isQValid(double q)
	{
		return std::isfinite(q)
			&& q >= (MIN_Q - EPSILON)
			&& q <= (MAX_Q + EPSILON);
	}

	inline bool isBwValid(double bwOct)
	{
		return std::isfinite(bwOct)
			&& bwOct >= (MIN_BW_OCT - EPSILON)
			&& bwOct <= (MAX_BW_OCT + EPSILON);
	}

	inline bool isSlopeValid(double slopeDb)
	{
		return std::isfinite(slopeDb)
			&& slopeDb >= (MIN_SLOPE_DB - EPSILON)
			&& slopeDb <= (MAX_SLOPE_DB + EPSILON);
	}
}
