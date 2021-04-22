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

#include "IFilter.h"
#include "BiQuad.h"

#pragma AVRT_VTABLES_BEGIN
class BiQuadFilter : public IFilter
{
public:
	BiQuadFilter(BiQuad::Type type, double dbGain, double freq, double bandwidthOrQOrS, bool isBandwidthOrS, bool isCornerFreq);
	virtual~BiQuadFilter();
	bool getInPlace() override { return true; }
	std::vector<std::wstring> initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames) override;
	void process(double** output, double** input, unsigned frameCount) override;

	BiQuad::Type getType() const;
	double getDbGain() const;
	double getFreq() const;
	double getBandwidthOrQOrS() const;
	bool getIsBandwidthOrS() const;
	bool getIsCornerFreq() const;

private:
	BiQuad::Type type;
	double dbGain;
	double freq;
	double bandwidthOrQOrS;
	bool isBandwidthOrS;
	bool isCornerFreq;

	size_t channelCount;
	BiQuad* biquads;
};
#pragma AVRT_VTABLES_END
