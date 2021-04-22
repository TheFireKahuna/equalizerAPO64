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
#include <vector>
#include "FilterEngine.h"
#include "VoicemeeterRemote.h"

class VoicemeeterClient
{
public:
	VoicemeeterClient(const std::vector<std::wstring>& outputs);
	~VoicemeeterClient();
	void start();
	void stop();

	void handle(long nCommand, void* lpData, long nnn);
	bool restart = false;

private:
	bool isBufferSilent(float** sampleData, long sampleCount);

	unsigned long mainThreadId;
	T_VBVMR_INTERFACE vmr;
	std::vector<FilterEngine*> engines;
	std::vector<int> idleSampleCounts;
	bool loggedIn = false;
	long voicemeeterType = 1;
	std::vector<std::wstring> outputs;
};
