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
#include <sstream>
#include <algorithm>

#include "helpers/MemoryHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/ChannelHelper.h"
#include "ChannelFilter.h"

using namespace std;

ChannelFilter::ChannelFilter(const vector<wstring>& words)
	: words(words)
{
}

ChannelFilter::~ChannelFilter()
{
}

vector<wstring> ChannelFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	size_t channelCount = channelNames.size();
	vector<bool> selectedChannels = vector<bool>(channelCount, false);

	for (vector<wstring>::iterator it = words.begin(); it != words.end(); it++)
	{
		wstring currentWord = *it;
		int channelNr = -1;

		if (currentWord == L"ALL")
			selectedChannels = vector<bool>(channelCount, true);
		else
			channelNr = ChannelHelper::getChannelIndex(currentWord, channelNames);

		if (channelNr != -1 && channelNr < (int)channelCount)
		{
			selectedChannels[channelNr] = true;
		}
	}

	vector<wstring> selectedChannelNames;
	for (unsigned i = 0; i < channelCount; i++)
	{
		if (selectedChannels[i])
			selectedChannelNames.push_back(channelNames[i]);
	}

	wstringstream channelNumbers;
	for (size_t c = 0; c < channelCount; c++)
	{
		if (selectedChannels[c])
		{
			if (channelNumbers.tellp() > 0)
				channelNumbers << L", ";

			channelNumbers << c + 1;
		}
	}

	TraceF(L"Selecting channel(s) number %s", channelNumbers.str().c_str());

	return selectedChannelNames;
}

#pragma AVRT_CODE_BEGIN
void ChannelFilter::process(double** output, double** input, unsigned frameCount)
{
	// nothing to do
}
#pragma AVRT_CODE_END
