/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

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
#include "helpers/StringHelper.h"
#include "helpers/VSTPluginLibrary.h"
#include "helpers/LogHelper.h"
#include "VSTPluginFilter.h"
#include "VSTPluginFilterFactory.h"

using namespace std;

vector<IFilter*> VSTPluginFilterFactory::createFilter(const std::wstring& configPath, std::wstring& command, std::wstring& parameters)
{
	VSTPluginFilter* filter = NULL;

	if (command == L"VSTPlugin")
	{
		shared_ptr<VSTPluginLibrary> library;
		wstring chunkData;
		unordered_map<wstring, float> paramMap;
		vector<wstring> parts = StringHelper::splitQuoted(parameters, ' ');
		for (unsigned i = 0; i + 1 < parts.size(); i += 2)
		{
			wstring key = parts[i];
			wstring value = parts[i + 1];

			if (key == L"Library")
			{
				wstring libPath;
				if (PathIsRelativeW(value.c_str()))
				{
					wchar_t filePath[MAX_PATH];
					wstring pluginPath = VSTPluginLibrary::getDefaultPluginPath();
					pluginPath._Copy_s(filePath, sizeof(filePath) / sizeof(wchar_t), MAX_PATH);
					if (pluginPath.size() < MAX_PATH)
						filePath[pluginPath.size()] = L'\0';
					else
						filePath[MAX_PATH - 1] = L'\0';
					PathAppendW(filePath, value.c_str());
					libPath = filePath;
				}
				else
					libPath = value;

				library = VSTPluginLibrary::getInstance(libPath);
			}
			else if (key == L"ChunkData")
			{
				chunkData = value;
			}
			else
			{
				float f = wcstof(value.c_str(), NULL);
				paramMap[key] = f;
			}
		}

		if (library != NULL)
		{
			bool create = true;
			if (configPath != L"")
			{
				create = false;
				TraceF(L"Adding VST plugin %s", library->getLibPath().c_str());
				int res = library->initialize();
				if (res < 0)
				{
#ifdef _WIN64
					int bitDepth = 64;
#else
					int bitDepth = 32;
#endif
					if (res == AbstractLibrary::FILE_NOT_FOUND)
						LogF(L"File %s not found", library->getLibPath());
					else if (res == AbstractLibrary::LOADING_FAILED)
						LogF(L"Library %s could not be loaded", library->getLibPath());
					else if (res == AbstractLibrary::FUNCTIONS_MISSING)
						LogF(L"Library %s does not contain needed functions", library->getLibPath());
					else if (res == AbstractLibrary::WRONG_ARCHITECTURE)
						LogF(L"Library %s has wrong architecture, must be %d-bit", library->getLibPath(), bitDepth);
				}
				else
				{
					create = true;
				}
			}

			if (create)
			{
				void* mem = MemoryHelper::alloc(sizeof(VSTPluginFilter));
				filter = new(mem) VSTPluginFilter(library, chunkData, paramMap);
			}
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
