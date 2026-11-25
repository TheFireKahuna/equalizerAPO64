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

#include <vector>
#include "FilterConfiguration.h"
#include "helpers/GainIterator.h"

class GainAnalysisHelper
{
public:
	// Analytically computes the exact maximum peak gain (in dB) of the filter chain
	// across [0 Hz .. Nyquist), independent of FFT window truncation.
	// Optionally populates outPlotNodes with a high-resolution frequency response curve
	// that includes exact critical points (BiQuad resonances and exact global peak).
	static double analyzeChain(
		const std::vector<FilterInfo*>& filterInfos,
		double sampleRate,
		std::vector<FilterNode>* outPlotNodes = nullptr);
};
