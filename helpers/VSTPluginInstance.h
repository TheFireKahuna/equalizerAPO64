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

#include <string>
#include <memory>
#include <functional>
#include "aeffectx.h"

class VSTPluginLibrary;

class VSTPluginInstance
{
public:
	VSTPluginInstance(const std::shared_ptr<VSTPluginLibrary>& library, int processLevel);
	~VSTPluginInstance();

	bool initialize();

	int numInputs() const;
	int numOutputs() const;
	bool canReplacing() const;
	int uniqueID() const;
	std::wstring getName() const;
	int getUsedChannelCount() const;
	void setUsedChannelCount(int count);
	float getSampleRate() const;
	int getProcessLevel() const;
	void setProcessLevel(int value);
	int getLanguage() const;
	void setLanguage(int value);
	bool canDoubleReplacing() const;
	int getInitialDelay() const;

	void prepareForProcessing(float sampleRate, int blockSize);
	void writeToEffect(const std::wstring& chunkData, const std::unordered_map<std::wstring, float>& paramMap);
	void readFromEffect(std::wstring& chunkData, std::unordered_map<std::wstring, float>& paramMap) const;

	void startProcessing();
	void processDoubleReplacing(double** inputArray, double** outputArray, int frameCount);
	void processReplacing(float** inputArray, float** outputArray, int frameCount);
	void process(float** inputArray, float** outputArray, int frameCount);
	void stopProcessing();

	void startEditing(HWND hWnd, short* width, short* height);
	void doIdle();
	void stopEditing();

	void setAutomateFunc(std::function<void()> func);
	void onAutomate();

	void setSizeWindowFunc(std::function<void(int, int)> func);
	void onSizeWindow(int w, int h);

private:
	std::shared_ptr<VSTPluginLibrary> library;
	vst_effect_t* effect = NULL;
	std::function<void()> automateFunc;
	std::function<void(int, int)> sizeWindowFunc;
	float sampleRate = 0.0f;
	int usedChannelCount = -1;
	int processLevel = 0;
	int language = 1;
};
