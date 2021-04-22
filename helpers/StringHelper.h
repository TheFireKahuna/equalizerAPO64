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

#pragma once

#include <string>
#include <vector>

class StringHelper
{
public:
	// replaces any occurrence of a character from chars in s with the replacement string
	static std::wstring replaceCharacters(const std::wstring& s, const std::wstring& chars, const std::wstring& replacement);
	static std::wstring replaceIllegalCharacters(const std::wstring& filename);
	static std::wstring toWString(const std::string& s, unsigned codepage);
	static std::wstring toLowerCase(const std::wstring& s);
	static std::wstring toUpperCase(const std::wstring& s);
	static std::wstring trim(const std::wstring& s);
	static std::vector<std::wstring> split(const std::wstring& s, wchar_t splitChar, bool skipEmpty = true);
	static std::wstring join(const std::vector<std::wstring>& strings, const std::wstring& separator);
	static std::wstring getSystemErrorString(long status);
	static std::vector<std::wstring> splitQuoted(const std::wstring& s, wchar_t splitChar, wchar_t quoteChar = '"');
};
