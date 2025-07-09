/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Alexander Walch

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
#define _USE_MATH_DEFINES
#include <cmath>
#include <regex>
#include <sstream>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "LoudnessCorrectionFilter.h"
#include "LoudnessCorrectionFilterFactory.h"

using namespace std;

LoudnessCorrectionFilterFactory::LoudnessCorrectionFilterFactory()
{
}

vector<IFilter*> LoudnessCorrectionFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	vector<IFilter*> allFilter(0);

	if (command == L"LoudnessCorrection")
	{
		LoudnessCorrectionFilter::FilterParameters filterParameters(parameters);
		if (filterParameters.isInitialized())
		{
			TraceF(L"Adding loudness correction filter");
			void* mem = MemoryHelper::alloc(sizeof(LoudnessCorrectionFilter));
			allFilter.push_back(new(mem) LoudnessCorrectionFilter(filterParameters));
		}
	}

	return allFilter;
}
