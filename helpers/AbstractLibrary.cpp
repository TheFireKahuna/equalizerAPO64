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
#include <Imagehlp.h>
#include "AbstractLibrary.h"
#include "helpers/LogHelper.h"

using namespace std;

AbstractLibrary::~AbstractLibrary()
{
	if (module != NULL)
	{
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(module, path, MAX_PATH);

		FreeLibrary(module);
		module = NULL;

		TraceF(L"Unloaded library %s", path);
	}
}

int AbstractLibrary::initialize()
{
	if (module == NULL)
	{
		wstring libPath = getLibPath();
		if (GetFileAttributesW(libPath.c_str()) == INVALID_FILE_ATTRIBUTES)
			return FILE_NOT_FOUND;
		module = LoadLibraryW(libPath.c_str());
		if (module == NULL)
		{
			unsigned short arch = getFileArchitecture(libPath);
#ifdef _WIN64
			int bitDepth = 64;
#else
			int bitDepth = 32;
#endif
			if (arch != 0 && (bitDepth == 64 && arch != IMAGE_FILE_MACHINE_AMD64
				|| bitDepth == 32 && arch != IMAGE_FILE_MACHINE_I386))
				return WRONG_ARCHITECTURE;

			return LOADING_FAILED;
		}

		if (!loadFunctions())
		{
			FreeLibrary(module);
			module = NULL;
			return FUNCTIONS_MISSING;
		}

		int res = customInitialize();
		if (res < 0)
			return res;

		TraceF(L"Loaded library %s", libPath.c_str());

		return 1;
	}

	return 0;
}

int AbstractLibrary::customInitialize()
{
	// overwrite if needed
	return 0;
}

unsigned short AbstractLibrary::getFileArchitecture(const wstring& filePath)
{
	unsigned short result = 0;

	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		HANDLE hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMap != NULL)
		{
			void* mapAddr = MapViewOfFileEx(hMap, FILE_MAP_READ, 0, 0, 0, NULL);
			if (mapAddr != NULL)
			{
				PIMAGE_NT_HEADERS ntHeaders = ImageNtHeader(mapAddr);
				if (ntHeaders != NULL)
					result = ntHeaders->FileHeader.Machine;
				UnmapViewOfFile(mapAddr);
			}
			CloseHandle(hMap);
		}
		CloseHandle(hFile);
	}

	return result;
}
