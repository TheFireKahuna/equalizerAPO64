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
#include <sstream>

#include "helpers/LogHelper.h"
#include "helpers/MemoryHelper.h"
#include "helpers/ChannelHelper.h"
#include "CopyFilter.h"

using namespace std;

CopyFilter::CopyFilter(const vector<Assignment>& assignments)
{
	this->assignments = assignments;

	internalAssignments = NULL;
	assignmentCount = 0;
}

CopyFilter::~CopyFilter()
{
	cleanup();
}

vector<wstring> CopyFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	cleanup();

	assignmentCount = (unsigned)assignments.size();
	internalAssignments = (InternalAssignment*)MemoryHelper::alloc(assignmentCount * sizeof(Assignment));

	vector<wstring> outChannelNames;

	for (unsigned i = 0; i < assignmentCount; i++)
	{
		InternalAssignment& ia = internalAssignments[i];
		Assignment& a = assignments[i];

		wstring channelName = a.targetChannel;
		int channelIndex = ChannelHelper::getChannelIndex(a.targetChannel, channelNames, true);
		if (channelIndex != -1)
			channelName = channelNames[channelIndex];
		vector<wstring>::const_iterator it = find(outChannelNames.begin(), outChannelNames.end(), channelName);
		ia.targetChannel = (int)(it - outChannelNames.begin());
		if (it == outChannelNames.end())
			outChannelNames.push_back(channelName);

		ia.sourceCount = (unsigned)a.sourceSum.size();
		ia.sourceSum = (InternalAssignment::InternalSummand*)MemoryHelper::alloc(ia.sourceCount * sizeof(InternalAssignment::InternalSummand));

		for (unsigned j = 0; j < ia.sourceCount; j++)
		{
			InternalAssignment::InternalSummand& is = ia.sourceSum[j];
			Assignment::Summand& s = a.sourceSum[j];

			if (s.channel != L"")
				is.channel = ChannelHelper::getChannelIndex(s.channel, channelNames);
			else
				is.channel = -1;

			if (s.isDecibel)
				is.factor = pow(10.0, s.factor / 20.0);
			else
				is.factor = s.factor;
		}
	}

	wstringstream stream;
	stream << "Copying ";
	for (unsigned i = 0; i < assignmentCount; i++)
	{
		InternalAssignment& ia = internalAssignments[i];
		if (i > 0)
			stream << ", ";
		stream << L"to channel " << outChannelNames[ia.targetChannel].c_str() << " ";
		for (unsigned j = 0; j < ia.sourceCount; j++)
		{
			InternalAssignment::InternalSummand& is = ia.sourceSum[j];
			if (j > 0)
				stream << ", ";
			if (is.channel != -1)
				stream << L"from channel " << channelNames[is.channel].c_str() << L" with factor " << is.factor;
			else
				stream << L"value " << is.factor;
		}
	}
	TraceF(L"%s", stream.str().c_str());

	return outChannelNames;
}

#pragma AVRT_CODE_BEGIN
void CopyFilter::process(double** output, double** input, unsigned frameCount)
{
	for (unsigned i = 0; i < assignmentCount; i++)
	{
		InternalAssignment& ia = internalAssignments[i];

		if (ia.targetChannel == -1 || ia.sourceCount == 0)
			continue;

		{
			InternalAssignment::InternalSummand& is = ia.sourceSum[0];

			if (is.channel == -1)
				for (unsigned f = 0; f < frameCount; f++)
					output[ia.targetChannel][f] = is.factor;
			else if (is.factor == 1.0)
				memcpy(output[ia.targetChannel], input[is.channel], frameCount * sizeof(double));
			else
				for (unsigned f = 0; f < frameCount; f++)
					output[ia.targetChannel][f] = is.factor * input[is.channel][f];
		}

		for (unsigned j = 1; j < ia.sourceCount; j++)
		{
			InternalAssignment::InternalSummand& is = ia.sourceSum[j];

			if (is.channel == -1)
				for (unsigned f = 0; f < frameCount; f++)
					output[ia.targetChannel][f] += is.factor;
			else if (is.factor == 1.0)
				for (unsigned f = 0; f < frameCount; f++)
					output[ia.targetChannel][f] += input[is.channel][f];
			else
				for (unsigned f = 0; f < frameCount; f++)
					output[ia.targetChannel][f] += is.factor * input[is.channel][f];
		}
	}
}
#pragma AVRT_CODE_END

void CopyFilter::cleanup()
{
	if (internalAssignments != NULL)
	{
		for (unsigned i = 0; i < assignmentCount; i++)
		{
			InternalAssignment& ia = internalAssignments[i];
			if (ia.sourceSum != NULL)
			{
				MemoryHelper::free(ia.sourceSum);
				ia.sourceSum = NULL;
			}
		}

		MemoryHelper::free(internalAssignments);
		internalAssignments = NULL;
	}
}

std::vector<Assignment> CopyFilter::getAssignments() const
{
	return assignments;
}
