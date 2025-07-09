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
#include <Shlwapi.h>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "ConvolutionFilter.h"
#include "ConvolutionFilterFactory.h"

using namespace std;

vector<IFilter*> ConvolutionFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	ConvolutionFilter* filter = NULL;

	if (command == L"Convolution")
	{
		wstring value = parameters;
		while (value.length() > 0 && iswspace(value[0]))
			value = value.substr(1);

		wstring absolutePath;
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
			absolutePath = filePath;
		}
		else
			absolutePath = value;

		void* mem = MemoryHelper::alloc(sizeof(ConvolutionFilter));
		filter = new(mem) ConvolutionFilter(absolutePath);
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
