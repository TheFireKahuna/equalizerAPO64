/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

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
#include <cwchar>
#include <regex>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/SafetyLimits.h"
#include "GraphicEQFilter.h"
#include "GraphicEQFilterFactory.h"

using namespace std;

static const wregex regexStrictPair(
	L"^\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\\s+([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\\s*$");

vector<IFilter*> GraphicEQFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	GraphicEQFilter* filter = NULL;

	if (command == L"GraphicEQ")
	{
		wstring trimmed = StringHelper::trim(parameters);
		vector<FilterNode> nodes;
		bool valid = true;

		if (!trimmed.empty())
		{
			wstring value = trimmed;
			if (value.find(L'.') == wstring::npos)
				value = StringHelper::replaceCharacters(value, L",", L".");

			// Split strictly on ';' without skipping empty segments in the middle
			size_t start = 0;
			while (start <= value.size())
			{
				size_t semiPos = value.find(L';', start);
				wstring segment = (semiPos == wstring::npos)
					? value.substr(start)
					: value.substr(start, semiPos - start);

				if (semiPos == wstring::npos && StringHelper::trim(segment).empty() && !nodes.empty())
				{
					// Allow single trailing semicolon at end of line
					break;
				}

				wsmatch match;
				if (!regex_match(segment, match, regexStrictPair))
				{
					LogF(L"Rejected malformed GraphicEQ node \"%s\"", segment.c_str());
					valid = false;
					break;
				}

				wstring freqStr = match.str(1);
				wstring gainStr = match.str(2);
				wchar_t* endFreq = NULL;
				wchar_t* endGain = NULL;
				double freq = wcstod(freqStr.c_str(), &endFreq);
				double gain = wcstod(gainStr.c_str(), &endGain);

				if (endFreq != freqStr.c_str() + freqStr.size()
					|| endGain != gainStr.c_str() + gainStr.size()
					|| !SafetyLimits::isFreqValid(freq)
					|| !SafetyLimits::isGainValid(gain))
				{
					LogF(L"Rejected out-of-range GraphicEQ node \"%s\" (allowed freq: %.0f..%.0f Hz, gain: %.1f..+%.1f dB)",
						segment.c_str(), SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ,
						SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);
					valid = false;
					break;
				}

				freq = std::clamp(freq, SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ);
				gain = std::clamp(gain, SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);
				nodes.push_back(FilterNode(freq, gain));

				if ((int)nodes.size() > SafetyLimits::MAX_GRAPHIC_EQ_NODES)
				{
					LogF(L"Rejected GraphicEQ exceeding %d nodes", SafetyLimits::MAX_GRAPHIC_EQ_NODES);
					valid = false;
					break;
				}

				if (semiPos == wstring::npos)
					break;
				start = semiPos + 1;
			}
		}

		if (valid)
		{
			sort(nodes.begin(), nodes.end());

			TraceF(L"Graphic equalizer with %d nodes", (int)nodes.size());

			void* mem = MemoryHelper::alloc(sizeof(GraphicEQFilter));
			filter = new(mem) GraphicEQFilter(nodes, 16384);
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
