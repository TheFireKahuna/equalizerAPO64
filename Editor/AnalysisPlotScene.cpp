/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

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

#include <cmath>
#include "AnalysisPlotScene.h"

using namespace std;

AnalysisPlotScene::AnalysisPlotScene(QObject* parent)
	: FrequencyPlotScene(parent)
{
}

void AnalysisPlotScene::setFreqData(fftwf_complex* freqData, int frameCount, unsigned sampleRate)
{
	nodes.clear();
	for (int i = 0; i < frameCount / 2; i++)
	{
		float freq = (i * 1.0f / frameCount) * sampleRate;
		// GainIterator can't handle 0 Hz node
		if (freq == 0.0f)
			freq = 0.001f;
		float gain = sqrt(freqData[i][0] * freqData[i][0] + freqData[i][1] * freqData[i][1]);
		float dbGain = log10(gain) * 20;
		nodes.push_back(FilterNode(freq, dbGain));
	}

	update();
}

const vector<FilterNode>& AnalysisPlotScene::getNodes() const
{
	return nodes;
}
