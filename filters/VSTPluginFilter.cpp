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

#include "stdafx.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "VSTPluginFilter.h"

using namespace std;

VSTPluginFilter::VSTPluginFilter(std::shared_ptr<VSTPluginLibrary> library, std::wstring chunkData, std::unordered_map<std::wstring, float> paramMap)
	: library(library), chunkData(chunkData), paramMap(paramMap)
{
	libPath = library->getLibPath();
}

VSTPluginFilter::~VSTPluginFilter()
{
	cleanup();
}

std::vector<std::wstring> VSTPluginFilter::initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames)
{
	cleanup();

	channelCount = channelNames.size();
	if (channelCount == 0)
		return channelNames;

	skipProcessing = false;

	void* mem = MemoryHelper::alloc(sizeof(VSTPluginInstance));
	VSTPluginInstance* firstEffect = new(mem) VSTPluginInstance(library, 2);
	if (!firstEffect->initialize())
	{
		LogF(L"The VST plugin %s crashed during initialization.", libPath.c_str());
		skipProcessing = true;
	}

	effectChannelCount = max(firstEffect->numInputs(), firstEffect->numOutputs());
	// round up
	effectCount = (channelCount + (effectChannelCount - 1)) / effectChannelCount;
	effects = (VSTPluginInstance**)MemoryHelper::alloc(effectCount * sizeof(VSTPluginInstance*));
	effects[0] = firstEffect;
	for (unsigned i = 1; i < effectCount; i++)
	{
		mem = MemoryHelper::alloc(sizeof(VSTPluginInstance));
		effects[i] = new(mem) VSTPluginInstance(library, 2);
		if (!effects[i]->initialize() && !skipProcessing)
		{
			LogF(L"The VST plugin %s crashed during initialization.", libPath.c_str());
			skipProcessing = true;
		}
	}

	prepareForProcessing(sampleRate, maxFrameCount);

	// 2 times for input and output
	emptyChannelCount = 2 * (effectCount * effectChannelCount - channelCount);
	emptyChannels = (double**)MemoryHelper::alloc(emptyChannelCount * sizeof(double*));
	for (unsigned i = 0; i < emptyChannelCount; i++)
	{
		emptyChannels[i] = (double*)MemoryHelper::alloc(maxFrameCount * sizeof(double));
		memset(emptyChannels[i], 0, maxFrameCount * sizeof(double));
	}

	inputArray = (double**)MemoryHelper::alloc(firstEffect->numInputs() * sizeof(double*));
	outputArray = (double**)MemoryHelper::alloc(firstEffect->numOutputs() * sizeof(double*));

	// Allocate float buffers for conversion
	if (firstEffect->numInputs() > 0) {
		floatInputs = (float**)MemoryHelper::alloc(firstEffect->numInputs() * sizeof(float*));
		_floatInputBuffer = (float*)MemoryHelper::alloc(firstEffect->numInputs() * maxFrameCount * sizeof(float));
		for (int i = 0; i < firstEffect->numInputs(); ++i) {
			floatInputs[i] = _floatInputBuffer + i * maxFrameCount;
		}
	}

	if (firstEffect->numOutputs() > 0) {
		floatOutputs = (float**)MemoryHelper::alloc(firstEffect->numOutputs() * sizeof(float*));
		_floatOutputBuffer = (float*)MemoryHelper::alloc(firstEffect->numOutputs() * maxFrameCount * sizeof(float));
		for (int i = 0; i < firstEffect->numOutputs(); ++i) {
			floatOutputs[i] = _floatOutputBuffer + i * maxFrameCount;
		}
	}

	// Allocate delay compensation buffers
	delayBufferLength = firstEffect->getInitialDelay();
	if (delayBufferLength > 0)
	{
		delayBuffers = (double**)MemoryHelper::alloc(channelCount * sizeof(double*));
		for (unsigned i = 0; i < channelCount; i++)
		{
			delayBuffers[i] = (double*)MemoryHelper::alloc(delayBufferLength * sizeof(double));
			memset(delayBuffers[i], 0, delayBufferLength * sizeof(double));
		}
		delayBufferOffset = 0;
	}

	return channelNames;
}

void VSTPluginFilter::prepareForProcessing(float sampleRate, unsigned maxFrameCount)
{
	__try
	{
		for (unsigned i = 0; i < effectCount; i++)
		{
			VSTPluginInstance* effect = effects[i];

			if (i == effectCount - 1 && (channelCount % effectChannelCount) != 0)
				effect->setUsedChannelCount(channelCount % effectChannelCount);
			else
				effect->setUsedChannelCount(effectChannelCount);
			effect->prepareForProcessing(sampleRate, maxFrameCount);
			effect->writeToEffect(chunkData, paramMap);
			effect->startProcessing();
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LogF(L"The VST plugin %s crashed while preparing for processing.", libPath.c_str());
		skipProcessing = true;
	}
}

#pragma AVRT_CODE_BEGIN
void convertFloatToDouble(double* dest, const float* src, size_t count);

// Converts a block of doubles back to floats.
void convertDoubleToFloat(float* dest, const double* src, size_t count);

void VSTPluginFilter::process(double** output, double** input, unsigned frameCount)
{
	if (skipProcessing)
	{
		for (unsigned i = 0; i < channelCount; i++)
			memcpy(output[i], input[i], frameCount * sizeof(double));
		return;
	}

	__try
	{
		unsigned channelOffset = 0;
		unsigned emptyChannelIndex = 0;
		for (unsigned i = 0; i < effectCount; i++)
		{
			VSTPluginInstance* effect = effects[i];
			// Setup double pointer arrays to point to the correct source/destination double buffers
			for (int j = 0; j < effect->numInputs(); j++)
			{
				if (channelOffset + j < channelCount)
					inputArray[j] = input[channelOffset + j];
				else
					inputArray[j] = emptyChannels[emptyChannelIndex++];
			}

			for (int j = 0; j < effect->numOutputs(); j++)
			{
				if (channelOffset + j < channelCount)
					outputArray[j] = output[channelOffset + j];
				else
					outputArray[j] = emptyChannels[emptyChannelIndex++];
			}

			if (effect->canDoubleReplacing()) {
				effect->processDoubleReplacing(inputArray, outputArray, frameCount);
			}
			else {
				// Convert input from double** to float** using pre-allocated buffers
				for (int j = 0; j < effect->numInputs(); j++)
				{
					convertDoubleToFloat(floatInputs[j], inputArray[j], frameCount);
				}

				if (effect->canReplacing())
				{
					effect->processReplacing(floatInputs, floatOutputs, frameCount);
				}
				else
				{
					// For non-replacing, VST expects to add to the output. Clear float buffer first.
					for (int j = 0; j < effect->numOutputs(); j++)
						memset(floatOutputs[j], 0, frameCount * sizeof(float));
					effect->process(floatInputs, floatOutputs, frameCount);
				}

				// Convert output from float** back to double** into the final destination
				for (int j = 0; j < effect->numOutputs(); j++)
				{
					convertFloatToDouble(outputArray[j], floatOutputs[j], frameCount);
				}
			}

			if (effect->numOutputs() < effect->numInputs())
			{
				for (int j = effect->numOutputs(); j < effect->numInputs(); j++)
				{
					if (channelOffset + j < channelCount)
						memset(output[channelOffset + j], 0, frameCount * sizeof(double));
				}
			}

			channelOffset += effectChannelCount;
		}

		// Apply delay compensation if needed
		if (delayBuffers != NULL && delayBufferLength > 0)
		{
			for (unsigned i = 0; i < channelCount; i++)
			{
				double* outputChannel = output[i];
				double* delayBuffer = delayBuffers[i];

				if (delayBufferLength <= frameCount)
				{
					// Delay is smaller than frame count - output from buffer then from current processing
					memcpy(outputChannel, delayBuffer + delayBufferOffset, (delayBufferLength - delayBufferOffset) * sizeof(double));
					memcpy(outputChannel + delayBufferLength - delayBufferOffset, delayBuffer, delayBufferOffset * sizeof(double));
					memcpy(outputChannel + delayBufferLength, outputChannel, (frameCount - delayBufferLength) * sizeof(double));
					memcpy(delayBuffer, outputChannel + frameCount - delayBufferLength, delayBufferLength * sizeof(double));
				}
				else
				{
					// Create temporary buffer to hold current output
					double* tempBuffer = (double*)MemoryHelper::alloc(frameCount * sizeof(double));
					memcpy(tempBuffer, outputChannel, frameCount * sizeof(double));

					if (delayBufferLength < delayBufferOffset + frameCount)
					{
						// Wrapping around the delay buffer
						memcpy(outputChannel, delayBuffer + delayBufferOffset, (delayBufferLength - delayBufferOffset) * sizeof(double));
						memcpy(outputChannel + delayBufferLength - delayBufferOffset, delayBuffer, (frameCount - (delayBufferLength - delayBufferOffset)) * sizeof(double));
						memcpy(delayBuffer + delayBufferOffset, tempBuffer, (delayBufferLength - delayBufferOffset) * sizeof(double));
						memcpy(delayBuffer, tempBuffer + delayBufferLength - delayBufferOffset, (frameCount - (delayBufferLength - delayBufferOffset)) * sizeof(double));
					}
					else
					{
						// Simple case - no wrapping
						memcpy(outputChannel, delayBuffer + delayBufferOffset, frameCount * sizeof(double));
						memcpy(delayBuffer + delayBufferOffset, tempBuffer, frameCount * sizeof(double));
					}

					MemoryHelper::free(tempBuffer);
				}
			}

			// Update buffer offset
			if (delayBufferLength <= frameCount)
				delayBufferOffset = 0;
			else
				delayBufferOffset = (delayBufferOffset + frameCount) % delayBufferLength;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		if (reportCrash)
		{
			LogF(L"The VST plugin %s crashed during audio processing.", libPath.c_str());
			reportCrash = false;
		}

		for (unsigned i = 0; i < channelCount; i++)
			memcpy(output[i], input[i], frameCount * sizeof(double));
	}
}
#pragma AVRT_CODE_END

std::shared_ptr<VSTPluginLibrary> VSTPluginFilter::getLibrary() const
{
	return library;
}

std::wstring VSTPluginFilter::getChunkData() const
{
	return chunkData;
}

std::unordered_map<std::wstring, float> VSTPluginFilter::getParamMap() const
{
	return paramMap;
}

void VSTPluginFilter::cleanup()
{
	if (effects != NULL)
	{
		for (unsigned i = 0; i < effectCount; i++)
		{
			VSTPluginInstance* effect = effects[i];
			effect->stopProcessing();
			effect->~VSTPluginInstance();
			MemoryHelper::free(effect);
		}
		MemoryHelper::free(effects);
		effects = NULL;
	}
	effectCount = 0;

	if (emptyChannels != NULL)
	{
		for (unsigned i = 0; i < emptyChannelCount; i++)
			MemoryHelper::free(emptyChannels[i]);
		MemoryHelper::free(emptyChannels);
		emptyChannels = NULL;
	}
	emptyChannelCount = 0;

	if (inputArray != NULL)
	{
		MemoryHelper::free(inputArray);
		inputArray = NULL;
	}

	if (outputArray != NULL)
	{
		MemoryHelper::free(outputArray);
		outputArray = NULL;
	}
    
    if (floatInputs != NULL) {
		MemoryHelper::free(floatInputs);
		floatInputs = NULL;
	}
	if (_floatInputBuffer != NULL) {
		MemoryHelper::free(_floatInputBuffer);
		_floatInputBuffer = NULL;
	}
	if (floatOutputs != NULL) {
		MemoryHelper::free(floatOutputs);
		floatOutputs = NULL;
	}
	if (_floatOutputBuffer != NULL) {
		MemoryHelper::free(_floatOutputBuffer);
		_floatOutputBuffer = NULL;
	}

	if (delayBuffers != NULL)
	{
		for (unsigned i = 0; i < channelCount; i++)
			MemoryHelper::free(delayBuffers[i]);
		MemoryHelper::free(delayBuffers);
		delayBuffers = NULL;
	}
	delayBufferLength = 0;
	delayBufferOffset = 0;
}
