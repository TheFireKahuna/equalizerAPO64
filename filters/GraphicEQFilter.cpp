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

#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#include <sndfile.h>

#include "helpers/LogHelper.h"
#include "helpers/MemoryHelper.h"
#include "GraphicEQFilter.h"

using namespace std;

GraphicEQFilter::GraphicEQFilter(const std::vector<FilterNode>& nodes, unsigned filterLength)
	: nodes(nodes), filterLength(filterLength)
{
	filters = NULL;
}

GraphicEQFilter::~GraphicEQFilter()
{
	cleanup();
}

vector<wstring> GraphicEQFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	cleanup();

	channelCount = (unsigned)channelNames.size();

	fftw_make_planner_thread_safe();
	fftw_complex* timeData = fftw_alloc_complex(2 * (size_t)filterLength);
	fftw_complex* freqData = fftw_alloc_complex(2 * (size_t)filterLength);
	fftw_plan planForward = fftw_plan_dft_1d(2 * (int)filterLength, timeData, freqData, FFTW_FORWARD, FFTW_ESTIMATE);
	fftw_plan planReverse = fftw_plan_dft_1d(2 * (int)filterLength, freqData, timeData, FFTW_BACKWARD, FFTW_ESTIMATE);

	GainIterator gainIterator(nodes);
	for (unsigned i = 0; i < filterLength; i++)
	{
		double freq = (double)i * 1.0 * (double)sampleRate / (2.0 * (double)filterLength);
		double dbGain = gainIterator.gainAt(freq);
		double gain = pow(10.0, dbGain / 20.0);

		freqData[i][0] = gain;
		freqData[i][1] = 0.0;
		freqData[2 * filterLength - i - 1][0] = gain;
		freqData[2 * filterLength - i - 1][1] = 0.0;
	}

	mps(timeData, freqData, planForward, planReverse);

	fftw_execute(planReverse);

	for (unsigned i = 0; i < 2 * filterLength; i++)
	{
		timeData[i][0] /= 2.0 * (double)filterLength;
		timeData[i][1] /= 2.0 * (double)filterLength;
	}

	for (unsigned i = 0; i < filterLength; i++)
	{
		double factor = 0.5 * (1.0 + cos(2.0 * M_PI * (double)i * 1.0 / (2.0 * (double)filterLength)));
		timeData[i][0] *= factor;
		timeData[i][1] *= factor;
	}

	double* buf = new double[filterLength];
	for (unsigned i = 0; i < filterLength; i++)
	{
		buf[i] = timeData[i][0];
	}

	fftw_free(timeData);
	fftw_free(freqData);
	fftw_destroy_plan(planForward);
	fftw_destroy_plan(planReverse);

	filters = (HConvSingle*)MemoryHelper::alloc(sizeof(HConvSingle) * channelCount);
	for (unsigned i = 0; i < channelCount; i++)
	{
		hcInitSingle(&filters[i], buf, filterLength, maxFrameCount, 1);
	}

	delete buf;

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void GraphicEQFilter::process(double** output, double** input, unsigned frameCount)
{
	if (filters == NULL)
		return;

	for (unsigned i = 0; i < channelCount; i++)
	{
		double* inputChannel = input[i];
		double* outputChannel = output[i];
		HConvSingle* filter = &filters[i];

		hcPutSingle(filter, inputChannel);
		hcProcessSingle(filter);
		hcGetSingle(filter, outputChannel);
	}
}
#pragma AVRT_CODE_END

const std::vector<FilterNode>& GraphicEQFilter::getNodes()
{
	return nodes;
}

void GraphicEQFilter::cleanup()
{
	if (filters != NULL)
	{
		for (unsigned i = 0; i < channelCount; i++)
			hcCloseSingle(&filters[i]);

		MemoryHelper::free(filters);
		filters = NULL;
	}
}

// Minimum phase spectrum from coefficients
void GraphicEQFilter::mps(fftw_complex* timeData, fftw_complex* freqData, fftw_plan planForward, fftw_plan planReverse)
{
	double threshold = pow(10.0, -100.0 / 20.0);
	double logThreshold = log(threshold);

	for (unsigned i = 0; i < filterLength * 2; i++)
	{
		if (freqData[i][0] < threshold)
			freqData[i][0] = logThreshold;
		else
			freqData[i][0] = log(freqData[i][0]);

		freqData[i][1] = 0.0;
	}

	fftw_execute(planReverse);

	for (unsigned i = 0; i < filterLength * 2; i++)
	{
		timeData[i][0] /= 2.0 * (double)filterLength;
		timeData[i][1] /= 2.0 * (double)filterLength;
	}

	for (unsigned i = 1; i < filterLength; i++)
	{
		timeData[i][0] += timeData[filterLength * 2 - i][0];
		timeData[i][1] -= timeData[filterLength * 2 - i][1];

		timeData[filterLength * 2 - i][0] = 0.0;
		timeData[filterLength * 2 - i][1] = 0.0;
	}
	timeData[filterLength][1] *= -1.0;

	fftw_execute(planForward);

	for (unsigned i = 0; i < filterLength * 2; i++)
	{
		double eR = exp(freqData[i][0]);
		freqData[i][0] = eR * cos(freqData[i][1]);
		freqData[i][1] = eR * sin(freqData[i][1]);
	}
}
