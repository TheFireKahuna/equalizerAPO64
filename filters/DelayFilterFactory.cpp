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

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "DelayFilter.h"
#include "DelayFilterFactory.h"

using namespace std;

vector<IFilter*> DelayFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	DelayFilter* filter = NULL;

	if (command == L"Delay")
	{
		// Conversion to period as decimal mark, if needed
		wstring value = StringHelper::replaceCharacters(parameters, L",", L".");

		double delay = -1;
		wstring unit;
		wstringstream stream(value);
		stream >> delay >> unit;

		if (delay >= 0)
		{
			if (StringHelper::toLowerCase(unit) == L"ms")
			{
				TraceF(L"Delaying by %g ms", delay);
				void* mem = MemoryHelper::alloc(sizeof(DelayFilter));
				filter = new(mem) DelayFilter(delay, true);
			}
			else if (StringHelper::toLowerCase(unit) == L"samples")
			{
				TraceF(L"Delaying by %g samples", delay);
				void* mem = MemoryHelper::alloc(sizeof(DelayFilter));
				filter = new(mem) DelayFilter(delay, false);
			}
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
