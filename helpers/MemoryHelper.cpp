/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2013  Jonas Thedering

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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#endif
#ifdef USE_WINDDK
// disable the declarations to use our own (see below)
#define AERT_Allocate _AERT_Allocate_disabled
#define AERT_Free _AERT_Free_disabled
#include <BaseAudioProcessingObject.h>
#undef AERT_Allocate
#undef AERT_Free
#endif

#include "LogHelper.h"
#include "MemoryHelper.h"

#ifdef USE_WINDDK
// someone forgot to add the __stdcall/WINAPI modifier to BaseAudioProcessingObject.h, so we can't use the existing declarations
extern "C"
{
HRESULT __stdcall AERT_Allocate(size_t size, void** pMemory);
HRESULT __stdcall AERT_Free(void* pMemory);
}
#endif

void* MemoryHelper::alloc(size_t size)
{
	void* memory;
	bool alternative = false;
	size += 16;
#ifdef USE_WINDDK
	HRESULT hr = AERT_Allocate(size, &memory);
	if (FAILED(hr))
	{
		size += 16;
		memory = malloc(size);

		alternative = true;
	}
#else
#ifdef _DEBUG
	memory = _malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__);
#else
	memory = malloc(size);
#endif
#endif
	if (memory == NULL)
	{
		LogFStatic(L"Allocation of %d bytes failed.", size);
		return NULL;
	}

	size_t offset = 16 - ((size_t)memory) % 16;
	if (alternative)
		offset += 16;
	void* ptr = ((char*)memory) + offset;
	((char*)ptr)[-1] = (char)offset;

	return ptr;
}

void MemoryHelper::free(void* ptr)
{
	bool alternative = false;
	char offset = ((char*)ptr)[-1];
	if (offset > 16)
		alternative = true;

	void* memory = ((char*)ptr) - offset;

#ifdef USE_WINDDK
	if (alternative)
	{
		::free(memory);
	}
	else
	{
		HRESULT hr = AERT_Free(memory);
		if (FAILED(hr))
		{
			LogFStatic(L"Memory release failed. Error code %X", hr);
		}
	}
#else
#ifdef _DEBUG
	_free_dbg(memory, _NORMAL_BLOCK);
#else
	::free(memory);
#endif
#endif
}