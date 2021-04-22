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
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#include <sndfile.h>
#include <fftw3.h>

#include "helpers/LogHelper.h"
#include "helpers/MemoryHelper.h"
#include "ConvolutionFilter.h"

using namespace std;

ConvolutionFilter::ConvolutionFilter(wstring filename)
{
	this->filename = filename;
	filters = NULL;
}

ConvolutionFilter::~ConvolutionFilter()
{
	cleanup();
}

vector<wstring> ConvolutionFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	cleanup();

	channelCount = (unsigned)channelNames.size();

	SF_INFO info;

	SNDFILE* inFile = sf_wchar_open(filename.c_str(), SFM_READ, &info);
	if (inFile == NULL)
	{
		LogF(L"Error while reading impulse response file: %S", sf_strerror(inFile));
	}
	else if (abs(sampleRate - info.samplerate) > 1.0f)
	{
		LogF(L"Impulse response sample rate (%d Hz) does not match device sample rate (%f Hz)", info.samplerate, sampleRate);
	}
	else
	{
		TraceF(L"Convolving using impulse response file %s", filename.c_str());
		unsigned fileChannelCount = info.channels;
		unsigned frameCount = (unsigned)info.frames;

		double* interleavedBuf = new double[frameCount * fileChannelCount];

		sf_count_t numRead = 0;
		while (numRead < frameCount)
			numRead += sf_readf_double(inFile, interleavedBuf + numRead * fileChannelCount, frameCount - numRead);

		sf_close(inFile);
		inFile = NULL;

		double** bufs = new double*[fileChannelCount];
		for (unsigned i = 0; i < fileChannelCount; i++)
		{
			double* buf = new double[frameCount];
			double* p = interleavedBuf + i;
			for (unsigned j = 0; j < frameCount; j++)
			{
				buf[j] = p[j * fileChannelCount];
			}

			bufs[i] = buf;
		}

		fftw_make_planner_thread_safe();
		filters = (HConvSingle*)MemoryHelper::alloc(sizeof(HConvSingle) * channelCount);
		for (unsigned i = 0; i < channelCount; i++)
		{
			hcInitSingle(&filters[i], bufs[i % fileChannelCount], frameCount, maxFrameCount, 1);
		}

		for (unsigned i = 0; i < fileChannelCount; i++)
		{
			delete[] bufs[i];
		}
		delete bufs;
		delete interleavedBuf;
	}

	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void ConvolutionFilter::process(double** output, double** input, unsigned frameCount)
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

void ConvolutionFilter::cleanup()
{
	if (filters != NULL)
	{
		for (unsigned i = 0; i < channelCount; i++) {
			hcCloseSingle(&filters[i]);
		}

		MemoryHelper::free(filters);
		filters = NULL;
	}
}
