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
#include <mpParser.h>
#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#ifndef NO_FILTERENGINE
#include "FilterEngine.h"
#endif
#include "DeviceFilterFactory.h"

using namespace std;

static wregex regexGuid(L"\\{[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\}");

#ifndef NO_FILTERENGINE
void DeviceFilterFactory::initialize(FilterEngine* engine)
{
	deviceString = engine->getDeviceString();

	mup::ParserX* parser = engine->getParser();
	parser->DefineConst(L"deviceName", engine->getDeviceName());
	parser->DefineConst(L"connectionName", engine->getConnectionName());
	parser->DefineConst(L"deviceGuid", engine->getDeviceGuid());
}
#endif

vector<IFilter*> DeviceFilterFactory::startOfConfiguration()
{
	deviceMatches = true;

	return vector<IFilter*>();
}

vector<IFilter*> DeviceFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	if (command == L"Device")
	{
		bool matches = matchDevice(deviceString, parameters);

		TraceF(L"%satching pattern \"%s\" with device \"%s\"", matches ? L"M" : L"Not m", StringHelper::trim(parameters).c_str(), deviceString.c_str());
		deviceMatches = matches;
	}

	if (!deviceMatches)
		// skip line for further factories
		command = L"";

	return vector<IFilter*>();
}

std::vector<IFilter*> DeviceFilterFactory::endOfFile(const std::wstring& configPath)
{
	// in outer file, the device must have matched, otherwise the inner file would not have been included
	deviceMatches = true;

	return vector<IFilter*>();
}

bool DeviceFilterFactory::matchDevice(const std::wstring& deviceString, const std::wstring& pattern)
{
	wstring value = StringHelper::trim(pattern) + L";";

	vector<vector<wstring>> fullList;
	vector<wstring> currentList;
	wstring currentWord;

	for (unsigned i = 0; i < value.length(); i++)
	{
		wchar_t c = value[i];
		if (c == L' ' || c == L';')
		{
			if (currentWord.length() > 0)
			{
				currentList.push_back(currentWord);
				currentWord.clear();
			}
			if (c == L';' && currentList.size() > 0)
			{
				fullList.push_back(currentList);
				currentList.clear();
			}
		}
		else
		{
			currentWord += c;
		}
	}

	wstring deviceStringNoGuid = regex_replace(deviceString, regexGuid, L"");

	bool matches = false;

	for (unsigned i = 0; i < fullList.size(); i++)
	{
		matches = true;

		if (fullList[i].size() == 1 && StringHelper::toLowerCase(fullList[i][0]) == L"all")
			break;

		for (unsigned j = 0; j < fullList[i].size(); j++)
		{
			wstring word = StringHelper::toLowerCase(fullList[i][j]);
			const wstring& matchString = word.find('{') == wstring::npos ? deviceStringNoGuid : deviceString;
			if (StringHelper::toLowerCase(matchString).find(word) == wstring::npos)
			{
				matches = false;
				break;
			}
		}

		if (matches)
			break;
	}

	return matches;
}
