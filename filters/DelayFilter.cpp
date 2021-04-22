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
#include <cmath>

#include "helpers/MemoryHelper.h"
#include "DelayFilter.h"

using namespace std;

DelayFilter::DelayFilter(double delay, bool isMs)
	: delay(delay), isMs(isMs)
{
	buffers = NULL;
}

DelayFilter::~DelayFilter()
{
	cleanup();
}

vector<wstring> DelayFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	cleanup();

	channelCount = (unsigned)channelNames.size();

	if (isMs)
		bufferLength = (unsigned)(sampleRate * delay / 1000.0 + 0.5);
	else
		bufferLength = (unsigned)(delay + 0.5);

	buffers = (double**)MemoryHelper::alloc(sizeof(double*) * channelCount);

	for (unsigned i = 0; i < channelCount; i++)
	{
		buffers[i] = (double*)MemoryHelper::alloc(sizeof(double) * bufferLength);
		memset(buffers[i], 0, sizeof(double) * bufferLength);
	}

	bufferOffset = 0;

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void DelayFilter::process(double** output, double** input, unsigned frameCount)
{
	for (unsigned i = 0; i < channelCount; i++)
	{
		double* inputChannel = input[i];
		double* outputChannel = output[i];
		double* bufferChannel = buffers[i];

		if (bufferLength <= frameCount)
		{
			memcpy(outputChannel, bufferChannel + bufferOffset, (bufferLength - bufferOffset) * sizeof(double));
			memcpy(outputChannel + bufferLength - bufferOffset, bufferChannel, bufferOffset * sizeof(double));
			memcpy(outputChannel + bufferLength, inputChannel, (frameCount - bufferLength) * sizeof(double));
			memcpy(bufferChannel, inputChannel + frameCount - bufferLength, bufferLength * sizeof(double));
		}
		else
		{
			if (bufferLength < bufferOffset + frameCount)
			{
				memcpy(outputChannel, bufferChannel + bufferOffset, (bufferLength - bufferOffset) * sizeof(double));
				memcpy(outputChannel + bufferLength - bufferOffset, bufferChannel, (frameCount - (bufferLength - bufferOffset)) * sizeof(double));
				memcpy(bufferChannel + bufferOffset, inputChannel, (bufferLength - bufferOffset) * sizeof(double));
				memcpy(bufferChannel, inputChannel + bufferLength - bufferOffset, (frameCount - (bufferLength - bufferOffset)) * sizeof(double));
			}
			else
			{
				memcpy(outputChannel, bufferChannel + bufferOffset, frameCount * sizeof(double));
				memcpy(bufferChannel + bufferOffset, inputChannel, frameCount * sizeof(double));
			}
		}
	}

	if (bufferLength <= frameCount)
		bufferOffset = 0;
	else
		bufferOffset = (bufferOffset + frameCount) % bufferLength;
}
#pragma AVRT_CODE_END

void DelayFilter::cleanup()
{
	if (buffers != NULL)
	{
		for (unsigned i = 0; i < channelCount; i++)
			MemoryHelper::free(buffers[i]);

		MemoryHelper::free(buffers);
		buffers = NULL;
	}
}

bool DelayFilter::getIsMs() const
{
	return isMs;
}

double DelayFilter::getDelay() const
{
	return delay;
}
