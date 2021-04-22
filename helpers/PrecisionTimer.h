/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2012  Jonas Thedering

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

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class PrecisionTimer
{
	LARGE_INTEGER freq;
	LARGE_INTEGER startCount;

public:
	PrecisionTimer()
	{
		QueryPerformanceFrequency(&freq);
	}

	void start()
	{
		QueryPerformanceCounter(&startCount);
	}

	double stop()
	{
		LARGE_INTEGER stopCount;
		QueryPerformanceCounter(&stopCount);

		return double(stopCount.QuadPart - startCount.QuadPart) / freq.QuadPart;
	}
};
