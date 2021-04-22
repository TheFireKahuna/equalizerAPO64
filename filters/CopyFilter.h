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

struct Assignment
{
	std::wstring targetChannel;

	struct Summand
	{
		double factor;
		bool isDecibel;
		std::wstring channel;
	};

	std::vector<Summand> sourceSum;
};

#pragma AVRT_VTABLES_BEGIN
class CopyFilter : public IFilter
{
public:
	CopyFilter(const std::vector<Assignment>& assignments);
	virtual ~CopyFilter();
	bool getAllChannels() override {return true;}
	bool getInPlace() override {return false;}
	std::vector<std::wstring> initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames) override;
	void process(double** output, double** input, unsigned frameCount) override;

	std::vector<Assignment> getAssignments() const;

private:
	void cleanup();

	std::vector<Assignment> assignments;

	struct InternalAssignment
	{
		int targetChannel;

		struct InternalSummand
		{
			double factor;
			int channel;
		};

		InternalSummand* sourceSum;
		unsigned sourceCount;
	};

	InternalAssignment* internalAssignments;
	unsigned assignmentCount;
};
#pragma AVRT_VTABLES_END
