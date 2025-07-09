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

#pragma once

#include <string>
#include <vector>

#include "IFilter.h"

class FilterEngine;

struct FilterInfo
{
	IFilter* filter;
	bool inPlace;
	size_t* inChannels;
	size_t inChannelCount;
	size_t* outChannels;
	size_t outChannelCount;
};

#pragma AVRT_VTABLES_BEGIN
class FilterConfiguration
{
public:
	FilterConfiguration(FilterEngine* engine, const std::vector<FilterInfo*>& filterInfos, unsigned allChannelCount);
	~FilterConfiguration();

	void read(float* input, unsigned frameCount);
	void read(float** input, unsigned frameCount);
	void process(unsigned frameCount);
	unsigned doTransition(FilterConfiguration* nextConfig, unsigned frameCount, unsigned transitionCounter, unsigned transitionLength);
	void write(float* output, unsigned frameCount);
	void write(float** output, unsigned frameCount);
	float** getOutputSamples() {return allSamples;}
	bool isEmpty();

private:
	unsigned realChannelCount;
	unsigned outputChannelCount;
	unsigned allChannelCount;
	float** allSamples;
	float** allSamples2;
	float** currentSamples;
	float** currentSamples2;
	FilterInfo** filterInfos;
	unsigned filterCount;
};
#pragma AVRT_VTABLES_END
