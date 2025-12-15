/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

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

#include "IFilter.h"
#include "helpers/VSTPluginLibrary.h"

#pragma AVRT_VTABLES_BEGIN
class VSTPluginFilter : public IFilter
{
public:
	VSTPluginFilter(std::shared_ptr<VSTPluginLibrary> library, std::wstring chunkData, std::unordered_map<std::wstring, float> paramMap);
	~VSTPluginFilter();

	bool getInPlace() override {return false;}
	std::vector<std::wstring> initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames) override;
	void prepareForProcessing(float sampleRate, unsigned maxFrameCount);
	void process(double** output, double** input, unsigned frameCount) override;

	std::shared_ptr<VSTPluginLibrary> getLibrary() const;
	std::wstring getChunkData() const;
	std::unordered_map<std::wstring, float> getParamMap() const;

private:
	void cleanup();

	std::shared_ptr<VSTPluginLibrary> library;
	std::wstring libPath;
	std::wstring chunkData;
	std::unordered_map<std::wstring, float> paramMap;
	size_t channelCount;
	unsigned effectChannelCount;
	size_t effectCount = 0;
	VSTPluginInstance** effects = NULL;
	size_t emptyChannelCount = 0;
	double** emptyChannels = NULL;
	double** inputArray = NULL;
	double** outputArray = NULL;

	// Buffers for float conversion
	float** floatInputs = NULL;
	float* _floatInputBuffer = NULL;
	float** floatOutputs = NULL;
	float* _floatOutputBuffer = NULL;

	// Delay compensation
	unsigned delayBufferLength = 0;
	double** delayBuffers = NULL;
	unsigned delayBufferOffset = 0;
	unsigned delayCompensationSamplesRemaining = 0;

	bool skipProcessing = false;
	bool reportCrash = true;
};
#pragma AVRT_VTABLES_END
