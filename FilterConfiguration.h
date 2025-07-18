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
#include <memory> // For std::unique_ptr

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

	void read(double* input, unsigned frameCount);
	void read(double** input, unsigned frameCount);
	void process(unsigned frameCount);
	unsigned doTransition(FilterConfiguration* nextConfig, unsigned frameCount, unsigned transitionCounter, unsigned transitionLength);
	void write(double* output, unsigned frameCount);
	void write(double** output, unsigned frameCount);
	
    // Return a raw pointer-to-pointer for compatibility with existing filter interfaces
    double** getOutputSamples() { return allSamplesPtrs.data(); }
	bool isEmpty() const;

private:
    std::vector<std::unique_ptr<double[]>> allSamples;
    std::vector<std::unique_ptr<double[]>> allSamples2;
    
    // Non-owning pointers for fast access during process()
    std::vector<double*> allSamplesPtrs;
    std::vector<double*> allSamples2Ptrs;
    std::vector<double*> currentSamples;
    std::vector<double*> currentSamples2;

    std::vector<FilterInfo*> filterInfos;

	unsigned realChannelCount;
	unsigned outputChannelCount;
	unsigned allChannelCount;
};
#pragma AVRT_VTABLES_END
