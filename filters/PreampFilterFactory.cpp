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
#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "PreampFilter.h"
#include "PreampFilterFactory.h"

using namespace std;

vector<IFilter*> PreampFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	PreampFilter* filter = NULL;

	if (command == L"Preamp")
	{
		// Conversion to period as decimal mark, if needed
		wstring value = StringHelper::replaceCharacters(parameters, L",", L".");

		double preamp_dB;
		int matched = swscanf_s(value.c_str(), L" %lf dB", &preamp_dB);
		if (matched == 1)
		{
			TraceF(L"Adjusting preamp by %g dB", preamp_dB);

			void* mem = MemoryHelper::alloc(sizeof(PreampFilter));
			filter = new(mem) PreampFilter(preamp_dB);
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
