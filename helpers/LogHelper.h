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

#include <string>
#include <cstdio>

#define TraceF(format, ...) LogHelper::log(__FILE__, __LINE__, this, true, format, __VA_ARGS__)
#define TraceFStatic(format, ...) LogHelper::log(__FILE__, __LINE__, NULL, true, format, __VA_ARGS__)
#define LogF(format, ...) LogHelper::log(__FILE__, __LINE__, this, false, format, __VA_ARGS__)
#define LogFStatic(format, ...) LogHelper::log(__FILE__, __LINE__, NULL, false, format, __VA_ARGS__)

class LogHelper
{
public:
	static void log(const char* file, int line, const void* caller, bool trace, const wchar_t* format, ...);
	static void reset();
	static void set(FILE* fp, bool enableTrace, bool compact, bool useConsoleColors);

private:
	static bool initialized;
	static std::wstring logPath;
	static bool enableTrace;
	static FILE* presetFP;
	static bool compact;
	static bool useConsoleColors;
};
