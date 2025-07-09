/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2014  Jonas Thedering

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

#include "stdafx.h"
#include "helpers/MemoryHelper.h"
#include "BiQuadFilter.h"

using namespace std;

BiQuadFilter::BiQuadFilter(BiQuad::Type type, double dbGain, double freq, double bandwidthOrQOrS, bool isBandwidthOrS, bool isCornerFreq)
	: type(type), dbGain(dbGain), freq(freq), bandwidthOrQOrS(bandwidthOrQOrS), isBandwidthOrS(isBandwidthOrS), isCornerFreq(isCornerFreq)
{
	channelCount = 0;
	biquads = NULL;
}

BiQuadFilter::~BiQuadFilter()
{
	if (biquads != NULL)
	{
		MemoryHelper::free(biquads);
		biquads = NULL;
	}
}

vector<wstring> BiQuadFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	this->channelCount = channelNames.size();
	biquads = (BiQuad*)MemoryHelper::alloc(channelCount * sizeof(BiQuad));
	double biquadFreq = freq;
	if (isCornerFreq && (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF))
	{
		double s = bandwidthOrQOrS;
		if (!isBandwidthOrS) // Q
		{
			double q = bandwidthOrQOrS;
			double a = pow(10, dbGain / 40);
			s = 1.0 / ((1.0 / (q * q) - 2.0) / (a + 1.0 / a) + 1.0);
		}

		// frequency adjustment for DCX2496
		double centerFreqFactor = pow(10.0, abs(dbGain) / 80.0 / s);
		if (type == BiQuad::LOW_SHELF)
			biquadFreq *= centerFreqFactor;
		else
			biquadFreq /= centerFreqFactor;
	}

	for (unsigned i = 0; i < channelCount; i++)
	{
		new(biquads + i)BiQuad(type, dbGain, biquadFreq, sampleRate, bandwidthOrQOrS, isBandwidthOrS);
	}

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void BiQuadFilter::process(float** output, float** input, unsigned frameCount)
{
	for (unsigned i = 0; i < channelCount; i++)
	{
		BiQuad bq = biquads[i];

		float* inputChannel = input[i];
		float* outputChannel = output[i];

		for (unsigned j = 0; j < frameCount; j++)
			outputChannel[j] = (float)bq.process(inputChannel[j]);

		bq.removeDenormals();
		biquads[i] = bq;
	}
}

BiQuad::Type BiQuadFilter::getType() const
{
	return type;
}

double BiQuadFilter::getDbGain() const
{
	return dbGain;
}

double BiQuadFilter::getFreq() const
{
	return freq;
}

double BiQuadFilter::getBandwidthOrQOrS() const
{
	return bandwidthOrQOrS;
}

bool BiQuadFilter::getIsBandwidthOrS() const
{
	return isBandwidthOrS;
}

bool BiQuadFilter::getIsCornerFreq() const
{
	return isCornerFreq;
}

#pragma AVRT_CODE_END
