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
#include <algorithm>

#include "FilterEngine.h"
#include "helpers/MemoryHelper.h"
#include "FilterConfiguration.h"

using namespace std;

FilterConfiguration::FilterConfiguration(FilterEngine* engine, const vector<FilterInfo*>& filterInfos, unsigned allChannelCount)
{
	this->allChannelCount = allChannelCount;
	realChannelCount = engine->getRealChannelCount();
	outputChannelCount = engine->getOutputChannelCount();
	unsigned maxFrameCount = engine->getMaxFrameCount();

	allSamples = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	for (size_t i = 0; i < allChannelCount; i++)
		allSamples[i] = (double*)MemoryHelper::alloc(maxFrameCount * sizeof(double));
	allSamples2 = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	for (size_t i = 0; i < allChannelCount; i++)
		allSamples2[i] = (double*)MemoryHelper::alloc(maxFrameCount * sizeof(double));
	currentSamples = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	currentSamples2 = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));

	filterCount = (unsigned)filterInfos.size();
	this->filterInfos = (FilterInfo**)MemoryHelper::alloc(filterCount * sizeof(FilterInfo*));
	for (size_t i = 0; i < filterCount; i++)
		this->filterInfos[i] = filterInfos[i];
}

FilterConfiguration::~FilterConfiguration()
{
	MemoryHelper::free(currentSamples2);
	MemoryHelper::free(currentSamples);

	for (size_t i = 0; i < allChannelCount; i++)
		MemoryHelper::free(allSamples2[i]);
	MemoryHelper::free(allSamples2);

	for (size_t i = 0; i < allChannelCount; i++)
		MemoryHelper::free(allSamples[i]);
	MemoryHelper::free(allSamples);

	for (size_t i = 0; i < filterCount; i++)
	{
		filterInfos[i]->filter->~IFilter();
		MemoryHelper::free(filterInfos[i]->filter);
		if (filterInfos[i]->inChannels != NULL)
			MemoryHelper::free(filterInfos[i]->inChannels);
		if (filterInfos[i]->outChannels != NULL)
			MemoryHelper::free(filterInfos[i]->outChannels);
		MemoryHelper::free(filterInfos[i]);
	}
	MemoryHelper::free(filterInfos);
}

#pragma AVRT_CODE_BEGIN
void FilterConfiguration::read(double* input, unsigned frameCount)
{
#define DEINTERLEAVE_MACRO(ccount)\
	{\
		for (size_t c = 0; c < ccount; c++)\
		{\
			double* sampleChannel = allSamples[c];\
			double* i2 = input + c;\
			for (size_t i = 0; i < frameCount; i++)\
			{\
				sampleChannel[i] = i2[i * ccount];\
			}\
		}\
	}

	switch (realChannelCount)
	{
	case 1:
		DEINTERLEAVE_MACRO(1)
		break;
	case 2:
		DEINTERLEAVE_MACRO(2)
		break;
	case 6:
		DEINTERLEAVE_MACRO(6)
		break;
	case 8:
		DEINTERLEAVE_MACRO(8)
		break;
	default:
		DEINTERLEAVE_MACRO(realChannelCount)
	}
}

void FilterConfiguration::read(double** input, unsigned frameCount)
{
	for (unsigned c = 0; c < realChannelCount; c++)
		memcpy(allSamples[c], input[c], frameCount * sizeof(double));
}

void FilterConfiguration::process(unsigned frameCount)
{
	for (unsigned c = realChannelCount; c < allChannelCount; c++)
		memset(allSamples[c], 0, frameCount * sizeof(double));

	// for real mono input and >= stereo output, upmix to stereo as the Windows audio system would do automatically if no APO was present
	if (realChannelCount == 1 && outputChannelCount >= 2)
		memcpy(allSamples[1], allSamples[0], frameCount * sizeof(double));

	for (size_t i = 0; i < filterCount; i++)
	{
		FilterInfo* filterInfo = filterInfos[i];
		for (size_t j = 0; j < filterInfo->inChannelCount; j++)
			currentSamples[j] = allSamples[filterInfo->inChannels[j]];
		if (filterInfo->inPlace)
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				currentSamples2[j] = allSamples[filterInfo->outChannels[j]];
		}
		else
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				currentSamples2[j] = allSamples2[filterInfo->outChannels[j]];
		}

		filterInfo->filter->process(currentSamples2, currentSamples, frameCount);

		if (!filterInfo->inPlace)
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				swap(allSamples[filterInfo->outChannels[j]], allSamples2[filterInfo->outChannels[j]]);
			swap(currentSamples, currentSamples2);
		}
	}
}

unsigned FilterConfiguration::doTransition(FilterConfiguration* nextConfig, unsigned frameCount, unsigned transitionCounter, unsigned transitionLength)
{
	double** currentSamples = allSamples;
	double** nextSamples = nextConfig->allSamples;

	for (unsigned f = 0; f < frameCount; f++)
	{
		double factor = 0.5f * (1.0f - cos(transitionCounter * (double)M_PI / transitionLength));
		if (transitionCounter >= transitionLength)
			factor = 1.0f;

		for (unsigned c = 0; c < outputChannelCount; c++)
			currentSamples[c][f] = currentSamples[c][f] * (1 - factor) + nextSamples[c][f] * factor;

		transitionCounter++;
	}

	return transitionCounter;
}

void FilterConfiguration::write(double* output, unsigned frameCount)
{
#define INTERLEAVE_MACRO(ccount)\
	for (size_t c = 0; c < ccount; c++)\
	{\
		double* sampleChannel = allSamples[c];\
		double* o2 = output + c;\
		for (unsigned i = 0; i < frameCount; i++)\
		{\
			o2[i * ccount] = sampleChannel[i];\
		}\
	}

	switch (outputChannelCount)
	{
	case 1:
		INTERLEAVE_MACRO(1)
		break;
	case 2:
		INTERLEAVE_MACRO(2)
		break;
	case 6:
		INTERLEAVE_MACRO(6)
		break;
	case 8:
		INTERLEAVE_MACRO(8)
		break;
	default:
		INTERLEAVE_MACRO(outputChannelCount)
	}
}

void FilterConfiguration::write(double** output, unsigned frameCount)
{
	for (unsigned i = 0; i < outputChannelCount; i++)
		memcpy(output[i], allSamples[i], frameCount * sizeof(double));
}
#pragma AVRT_CODE_END

bool FilterConfiguration::isEmpty()
{
	return filterCount == 0;
}
