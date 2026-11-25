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
#include <cwchar>
#include <regex>
#include <sstream>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/SafetyLimits.h"
#include "BiQuadFilter.h"
#include "BiQuadFilterFactory.h"

using namespace std;

static const wregex regexCommand(L"^Filter(?:\\s+[0-9]+)?$");
static const wregex regexHeader(L"^\\s*ON\\s+([A-Za-z]+)(?=\\s|$)");
static const wregex regexLeadingSlope(L"^\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\\s*[dD][bB](?=\\s|$)");
static const wregex regexClauseFreq(L"^\\s*Fc\\s*([-+]?[0-9.\u00A0eE]+)\\s*H\\s*z(?=\\s|$)");
static const wregex regexClauseGain(L"^\\s*Gain\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)\\s*[dD][bB](?=\\s|$)");
static const wregex regexClauseQ(L"^\\s*Q\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)(?=\\s|$)");
static const wregex regexClauseBW(L"^\\s*BW\\s+Oct\\s*([-+]?(?:[0-9]+(?:\\.[0-9]+)?|\\.[0-9]+)(?:[eE][-+]?[0-9]+)?)(?=\\s|$)");

static bool parseStrictDouble(const wstring& str, double& outVal)
{
	if (str.empty())
		return false;
	wchar_t* endPtr = NULL;
	outVal = wcstod(str.c_str(), &endPtr);
	return endPtr == str.c_str() + str.size() && std::isfinite(outVal);
}

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

	if (regex_match(command, regexCommand))
	{
		// Conversion to period as decimal mark, if needed
		wstring rest = StringHelper::replaceCharacters(parameters, L",", L".");

		wsmatch match;
		if (regex_search(rest, match, regexHeader))
		{
			wstring typeString = match.str(1);
			auto typeIt = filterNameToTypeMap.find(typeString);
			if (typeIt != filterNameToTypeMap.end())
			{
				BiQuad::Type type = typeIt->second;
				wstring typeDescription = filterTypeToDescriptionMap[type];
				rest = match.suffix().str();

				double freq = 0.0;
				double gain = 0.0;
				double bandwidthOrQOrS = 0.0;
				bool isBandwidthOrS = false;
				bool isCornerFreq = false;

				bool hasFreq = false;
				bool hasGain = false;
				bool hasQ = false;
				bool hasBW = false;
				bool hasSlope = false;
				bool error = false;

				// Optional leading shelf slope: e.g. "ON LS 12 dB Fc 100 Hz Gain 0 dB"
				if (regex_search(rest, match, regexLeadingSlope))
				{
					double slopeVal = 0.0;
					if (!(type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
						|| !parseStrictDouble(match.str(1), slopeVal)
						|| !SafetyLimits::isSlopeValid(slopeVal))
					{
						LogF(L"Invalid or out-of-range shelf slope in filter: %s", parameters.c_str());
						error = true;
					}
					else
					{
						slopeVal = std::clamp(slopeVal, SafetyLimits::MIN_SLOPE_DB, SafetyLimits::MAX_SLOPE_DB);
						bandwidthOrQOrS = slopeVal;
						isBandwidthOrS = true;
						hasSlope = true;
						rest = match.suffix().str();
					}
				}

				// Parse remaining clauses strictly from left to right; reject any unrecognized or duplicate tokens
				while (!error)
				{
					wstring trimmedRest = StringHelper::trim(rest);
					if (trimmedRest.empty())
						break;

					if (regex_search(rest, match, regexClauseFreq))
					{
						if (hasFreq)
						{
							error = true;
							break;
						}
						freq = getFreq(match.str(1));
						if (!SafetyLimits::isFreqValid(freq))
						{
							LogF(L"Rejected out-of-range frequency %g Hz in filter: %s", freq, parameters.c_str());
							error = true;
							break;
						}
						freq = std::clamp(freq, SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ);
						hasFreq = true;
						rest = match.suffix().str();
					}
					else if (regex_search(rest, match, regexClauseGain))
					{
						bool supportsGain = (type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF);
						if (hasGain || !supportsGain || !parseStrictDouble(match.str(1), gain) || !SafetyLimits::isGainValid(gain))
						{
							LogF(L"Rejected invalid or out-of-range Gain in filter: %s", parameters.c_str());
							error = true;
							break;
						}
						gain = std::clamp(gain, SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);
						hasGain = true;
						rest = match.suffix().str();
					}
					else if (regex_search(rest, match, regexClauseQ))
					{
						double qVal = 0.0;
						if (hasQ || hasBW || hasSlope || !parseStrictDouble(match.str(1), qVal) || !SafetyLimits::isQValid(qVal))
						{
							LogF(L"Rejected invalid or out-of-range Q in filter: %s", parameters.c_str());
							error = true;
							break;
						}
						bandwidthOrQOrS = std::clamp(qVal, SafetyLimits::MIN_Q, SafetyLimits::MAX_Q);
						isBandwidthOrS = false;
						hasQ = true;
						rest = match.suffix().str();
					}
					else if (regex_search(rest, match, regexClauseBW))
					{
						double bwVal = 0.0;
						bool forbiddenShelf = (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF);
						if (hasQ || hasBW || hasSlope || forbiddenShelf || !parseStrictDouble(match.str(1), bwVal) || !SafetyLimits::isBwValid(bwVal))
						{
							LogF(L"Rejected invalid or out-of-range BW Oct in filter: %s", parameters.c_str());
							error = true;
							break;
						}
						bandwidthOrQOrS = std::clamp(bwVal, SafetyLimits::MIN_BW_OCT, SafetyLimits::MAX_BW_OCT);
						isBandwidthOrS = true;
						hasBW = true;
						rest = match.suffix().str();
					}
					else
					{
						LogF(L"Rejected unknown or malformed token in filter: %s", parameters.c_str());
						error = true;
						break;
					}
				}

				if (!error && !hasFreq)
				{
					LogF(L"Missing frequency in filter: %s", parameters.c_str());
					error = true;
				}

				if (!error && (type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF) && !hasGain)
				{
					LogF(L"Missing gain in filter: %s", parameters.c_str());
					error = true;
				}

				if (!error && (typeString == L"LPQ" || typeString == L"HPQ") && !(hasQ || hasBW))
				{
					LogF(L"Missing Q in %s filter: %s", typeString.c_str(), parameters.c_str());
					error = true;
				}

				if (!error)
				{
					if (!(hasQ || hasBW || hasSlope))
					{
						if (type == BiQuad::PEAKING || type == BiQuad::ALL_PASS)
						{
							LogF(L"No Q or bandwidth given in filter: %s", parameters.c_str());
							error = true;
						}
						else if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS)
						{
							bandwidthOrQOrS = M_SQRT1_2;
						}
						else if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
						{
							bandwidthOrQOrS = 0.9; // fixed S for RoomEQWizard / E-APO default
							isBandwidthOrS = true;
						}
						else if (type == BiQuad::NOTCH)
						{
							bandwidthOrQOrS = 30.0;
						}
					}
					else if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
					{
						if (isBandwidthOrS)
							bandwidthOrQOrS /= 12.0; // Maximum S is 1 for 12 dB
						if (typeString[typeString.length() - 1] != L'C')
							isCornerFreq = true;
					}
				}

				if (!error)
				{
					TraceF(L"Adding validated %s filter (Fc=%g Hz, Gain=%g dB)", typeDescription.c_str(), freq, gain);

					void* mem = MemoryHelper::alloc(sizeof(BiQuadFilter));
					filter = new(mem) BiQuadFilter(type, gain, freq, bandwidthOrQOrS, isBandwidthOrS, isCornerFreq);
				}
			}
			else
			{
				LogF(L"Invalid filter type %s", typeString.c_str());
			}
		}
		else
		{
			LogF(L"Invalid Filter syntax: %s", parameters.c_str());
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}

double BiQuadFilterFactory::getFreq(const wstring& freqString)
{
	// remove thousand's separator for locales utilizing non-breaking space
	wstring s = StringHelper::replaceCharacters(freqString, L"\u00A0", L"");
	if (s.empty())
		return -1.0;

	// Reject multiple dots (e.g. "1.000.000")
	size_t firstDot = s.find(L'.');
	if (firstDot != wstring::npos && s.find(L'.', firstDot + 1) != wstring::npos)
		return -1.0;

	double result = 0.0;
	if (!parseStrictDouble(s, result))
		return -1.0;

	if (s.length() >= 5 && s.find_first_of(L"eE") == wstring::npos)
	{
		if (s[s.length() - 4] == L'.')
		{
			// Interpret as thousands separator because of Room EQ Wizard (e.g. 1.000 Hz -> 1000 Hz)
			result *= 1000.0;
		}
	}

	return result;
}
