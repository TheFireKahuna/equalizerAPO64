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
#include <regex>
#include <cwchar>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/SafetyLimits.h"
#include "PreampFilter.h"
#include "PreampFilterFactory.h"

using namespace std;

static const wregex regexStrictPreamp(
	L"^\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\\s*[dD][bB]\\s*$");

vector<IFilter*> PreampFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	PreampFilter* filter = NULL;

	if (command == L"Preamp")
	{
		// Conversion to period as decimal mark, if needed
		wstring value = StringHelper::replaceCharacters(parameters, L",", L".");

		wsmatch match;
		if (regex_match(value, match, regexStrictPreamp))
		{
			wchar_t* endPtr = NULL;
			wstring numStr = match.str(1);
			double preamp_dB = wcstod(numStr.c_str(), &endPtr);
			if (endPtr == numStr.c_str() + numStr.size() && SafetyLimits::isGainValid(preamp_dB))
			{
				preamp_dB = std::clamp(preamp_dB, SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);
				TraceF(L"Adjusting preamp by %g dB", preamp_dB);

				void* mem = MemoryHelper::alloc(sizeof(PreampFilter));
				filter = new(mem) PreampFilter(preamp_dB);
			}
			else
			{
				LogF(L"Rejected out-of-range Preamp value \"%s\" (allowed: %.1f .. +%.1f dB)",
					parameters.c_str(), SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);
			}
		}
		else
		{
			LogF(L"Rejected malformed Preamp syntax \"%s\"", parameters.c_str());
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
