/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2022  Jonas Thedering

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
#include "FilterEngine.h"
#include "VoicemeeterRemote.h"

class VoicemeeterClient
{
public:
	VoicemeeterClient(const std::vector<std::wstring>& outputs);
	~VoicemeeterClient();
	void run();
	void handle(long nCommand, void* lpData, long nnn);

private:
	void initSoftware();
	void detectVoicemeeterType();
	void endSoftware();
	void handleCommand(WPARAM wparam, LPARAM lparam);
	bool isBufferSilent(float** sampleData, long sampleCount);

	std::vector<std::wstring> outputs;
	unsigned long mainThreadId;
	T_VBVMR_INTERFACE vmr;
	size_t wTimer = 0;
	bool connected = true;
	float sampleRate = 0.0f;
	unsigned maxFrameCount = 0;

	std::vector<FilterEngine*> engines;
	std::vector<int> idleSampleCounts;
};

class InitError
{
public:
	InitError(const std::wstring& message)
		: message(message)
	{
	}

	std::wstring getMessage()
	{
		return message;
	}

private:
	std::wstring message;
};
