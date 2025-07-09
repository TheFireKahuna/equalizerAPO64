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
#define _USE_MATH_DEFINES
#include <cmath>
#include <regex>
#include <sstream>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "BiQuadFilter.h"
#include "BiQuadFilterFactory.h"

using namespace std;

static wregex regexType(L"^\\s*ON\\s+([A-Za-z]+)");
static wregex regexFreq(L"\\s+Fc\\s*([-+0-9.eE\u00A0]+)\\s*H\\s*z");
static wregex regexGain(L"\\s+Gain\\s*([-+0-9.eE]+)\\s*dB");
static wregex regexQ(L"\\s+Q\\s*([-+0-9.eE]+)");
static wregex regexBW(L"\\s+BW\\s+Oct\\s*([-+0-9.eE]+)");
static wregex regexSlope(L"^\\s*([-+0-9.eE]+)\\s*dB");

BiQuadFilterFactory::BiQuadFilterFactory()
{
	filterNameToTypeMap[L"PK"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"PEQ"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"Modal"] = BiQuad::PEAKING;
	filterNameToTypeMap[L"LP"] = BiQuad::LOW_PASS;
	filterNameToTypeMap[L"HP"] = BiQuad::HIGH_PASS;
	filterNameToTypeMap[L"LPQ"] = BiQuad::LOW_PASS;
	filterNameToTypeMap[L"HPQ"] = BiQuad::HIGH_PASS;
	filterNameToTypeMap[L"BP"] = BiQuad::BAND_PASS;
	filterNameToTypeMap[L"LS"] = BiQuad::LOW_SHELF;
	filterNameToTypeMap[L"HS"] = BiQuad::HIGH_SHELF;
	filterNameToTypeMap[L"LSC"] = BiQuad::LOW_SHELF;
	filterNameToTypeMap[L"HSC"] = BiQuad::HIGH_SHELF;
	filterNameToTypeMap[L"NO"] = BiQuad::NOTCH;
	filterNameToTypeMap[L"AP"] = BiQuad::ALL_PASS;

	filterTypeToDescriptionMap[BiQuad::PEAKING] = L"peaking";
	filterTypeToDescriptionMap[BiQuad::LOW_PASS] = L"low-pass";
	filterTypeToDescriptionMap[BiQuad::HIGH_PASS] = L"high-pass";
	filterTypeToDescriptionMap[BiQuad::BAND_PASS] = L"band-pass";
	filterTypeToDescriptionMap[BiQuad::LOW_SHELF] = L"low-shelf";
	filterTypeToDescriptionMap[BiQuad::HIGH_SHELF] = L"high-shelf";
	filterTypeToDescriptionMap[BiQuad::NOTCH] = L"notch";
	filterTypeToDescriptionMap[BiQuad::ALL_PASS] = L"all-pass";
}

vector<IFilter*> BiQuadFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	BiQuadFilter* filter = NULL;

	if (command.find(L"Filter") == 0)
	{
		// Conversion to period as decimal mark, if needed
		parameters = StringHelper::replaceCharacters(parameters, L",", L".");

		wsmatch match;
		wstring typeString;

		bool found = regex_search(parameters, match, regexType);
		if (found)
		{
			typeString = match.str(1);
			if (filterNameToTypeMap.find(typeString) != filterNameToTypeMap.end())
			{
				BiQuad::Type type = filterNameToTypeMap[typeString];
				wstring typeDescription = filterTypeToDescriptionMap[type];
				parameters = match.suffix().str();

				wstringstream stream;
				stream << L"Adding " << typeDescription << L" filter";

				double freq = 0;
				double gain = 0;
				double bandwidthOrQOrS = 0;
				bool isBandwidthOrS = false;
				bool isCornerFreq = false;
				bool error = false;

				found = regex_search(parameters, match, regexFreq);
				if (found)
				{
					wstring freqString = match.str(1);
					freq = getFreq(freqString);
					stream << " with frequency " << freq << " Hz";
				}
				else
				{
					LogF(L"No frequency given in filter string %s%s", typeString.c_str(), parameters.c_str());
					error = true;
				}

				found = regex_search(parameters, match, regexGain);
				if (found)
				{
					if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::NOTCH || type == BiQuad::ALL_PASS)
						TraceF(L"Ignoring gain for filter of type %s", typeDescription.c_str());
					else
					{
						wstring gainString = match.str(1);
						gain = wcstod(gainString.c_str(), NULL);
						if (type == BiQuad::PEAKING)
							stream << ", gain " << gain << " dB";
						else
							stream << " and gain " << gain << " dB";
					}
				}
				else if (type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
				{
					LogF(L"No gain given in filter string %s%s", typeString.c_str(), parameters.c_str());
					error = true;
				}

				found = regex_search(parameters, match, regexQ);
				if (found)
				{
					wstring qString = match.str(1);
					bandwidthOrQOrS = wcstod(qString.c_str(), NULL);
					stream << " and Q " << bandwidthOrQOrS;
				}

				found = regex_search(parameters, match, regexBW);
				if (found)
				{
					if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
						TraceF(L"Ignoring bandwidth for filter of type %s", typeDescription.c_str());
					else
					{
						wstring bwString = match.str(1);
						bandwidthOrQOrS = wcstod(bwString.c_str(), NULL);
						isBandwidthOrS = true;
						stream << " and bandwidth " << bandwidthOrQOrS << " octaves";
					}
				}

				found = regex_search(parameters, match, regexSlope);
				if (found)
				{
					if (!(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF))
						TraceF(L"Ignoring slope for filter of type %s", typeDescription.c_str());
					else
					{
						wstring slopeString = match.str(1);
						bandwidthOrQOrS = wcstod(slopeString.c_str(), NULL);
						isBandwidthOrS = true;
						stream << " and slope " << bandwidthOrQOrS << " dB";
					}
				}

				if (bandwidthOrQOrS == 0)
				{
					if (type == BiQuad::PEAKING || type == BiQuad::ALL_PASS)
					{
						LogF(L"No Q or bandwidth given in filter string %s%s", typeString.c_str(), parameters.c_str());
						error = true;
					}
					else if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS)
					{
						bandwidthOrQOrS = M_SQRT1_2;
					}
					else if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
					{
						bandwidthOrQOrS = 0.9; // found out by experimentation with RoomEQWizard
						isBandwidthOrS = true;
					}
					else if (type == BiQuad::NOTCH)
					{
						bandwidthOrQOrS = 30.0; // found out by experimentation with RoomEQWizard
					}
				}
				else if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
				{
					if (isBandwidthOrS)
						// Maximum S is 1 for 12 dB
						bandwidthOrQOrS /= 12.0;
					if (typeString[typeString.length() - 1] != L'C')
						isCornerFreq = true;
				}

				if (!error)
				{
					TraceF(L"%s", stream.str().c_str());

					void* mem = MemoryHelper::alloc(sizeof(BiQuadFilter));
					filter = new(mem) BiQuadFilter(type, gain, freq, bandwidthOrQOrS, isBandwidthOrS, isCornerFreq);
				}
			}
			else if (typeString != L"None")
			{
				LogF(L"Invalid filter type %s", typeString.c_str());
			}
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}

double BiQuadFilterFactory::getFreq(const wstring& freqString)
{
	double result;
	// remove thousand's separator for locales utilizing non-breaking space
	wstring s = StringHelper::replaceCharacters(freqString, L"\u00A0", L"");
	int matched = swscanf_s(s.c_str(), L"%lf", &result);
	if (matched == 1)
	{
		if (s.length() >= 5 && s.find_first_of(L"eE") == wstring::npos)
		{
			if (s[s.length() - 4] == L'.')
			{
				// Interpret as thousands separator because of Room EQ Wizard
				result *= 1000.0;
			}
		}

		return result;
	}
	else
		return -1.0;
}
