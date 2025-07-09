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
#include <Shlwapi.h>

#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "FilterEngine.h"
#include "IncludeFilterFactory.h"

using namespace std;

const int RECURSION_LIMIT = 100;

void IncludeFilterFactory::initialize(FilterEngine* engine)
{
	this->engine = engine;
}

vector<IFilter*> IncludeFilterFactory::startOfConfiguration()
{
	recursionDepth = -1;

	return vector<IFilter*>();
}

vector<IFilter*> IncludeFilterFactory::startOfFile(const wstring& configPath)
{
	recursionDepth++;

	return vector<IFilter*>();
}

vector<IFilter*> IncludeFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	if (command == L"Include")
	{
		wstring value = parameters;
		while (value.length() > 0 && iswspace(value[0]))
			value = value.substr(1);

		wstring includePath;
		if (PathIsRelativeW(value.c_str()))
		{
			wchar_t filePath[MAX_PATH];
			configPath._Copy_s(filePath, sizeof(filePath) / sizeof(wchar_t), MAX_PATH);
			if (configPath.size() < MAX_PATH)
				filePath[configPath.size()] = L'\0';
			else
				filePath[MAX_PATH - 1] = L'\0';
			PathRemoveFileSpecW(filePath);
			PathAppendW(filePath, value.c_str());
			includePath = filePath;
		}
		else
			includePath = value;

		if (recursionDepth >= RECURSION_LIMIT)
			LogF(L"Skipping include of %s as recursion limit of %d has been reached", value.c_str(), RECURSION_LIMIT);
		else
			engine->loadConfigFile(includePath);
		command = L"";
	}

	return vector<IFilter*>();
}

std::vector<IFilter*> IncludeFilterFactory::endOfFile(const wstring& configPath)
{
	recursionDepth--;

	return vector<IFilter*>();
}
