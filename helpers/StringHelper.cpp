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
#include <string>
#include <sstream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "StringHelper.h"

using namespace std;

wstring StringHelper::replaceCharacters(const wstring& s, const wstring& chars, const wstring& replacement)
{
	wstring result;
	result.reserve(s.length());

	for (unsigned i = 0; i < s.length(); i++)
	{
		wchar_t c = s[i];
		if (chars.find(c) == -1)
			result += c;
		else
			result += replacement;
	}

	return result;
}

wstring StringHelper::replaceIllegalCharacters(const wstring& filename)
{
	return replaceCharacters(filename, L"<>:\"/\\|?*", L"_");
}

wstring StringHelper::toWString(const string& s, unsigned codepage)
{
	int length = MultiByteToWideChar(codepage, 0, s.c_str(), -1, NULL, 0);
	wchar_t* charBuf = new wchar_t[length];
	MultiByteToWideChar(codepage, 0, s.c_str(), -1, charBuf, length);
	wstring result = charBuf;
	delete charBuf;

	return result;
}

string StringHelper::toString(const wstring& s, unsigned codepage)
{
	int length = WideCharToMultiByte(codepage, 0, s.c_str(), -1, NULL, 0, NULL, NULL);
	char* charBuf = new char[length];
	WideCharToMultiByte(codepage, 0, s.c_str(), -1, charBuf, length, NULL, NULL);
	string result = charBuf;
	delete charBuf;

	return result;
}

wstring StringHelper::toLowerCase(const wstring& s)
{
	wchar_t* charBuf = new wchar_t[s.length() + 1];
	memcpy(charBuf, s.c_str(), (s.length() + 1) * sizeof(wchar_t));
	errno_t err = _wcslwr_s(charBuf, s.length() + 1);

	wstring result = charBuf;
	delete charBuf;

	if (err == 0)
		return result;
	else
		return s;
}

wstring StringHelper::toUpperCase(const wstring& s)
{
	wchar_t* charBuf = new wchar_t[s.length() + 1];
	memcpy(charBuf, s.c_str(), (s.length() + 1) * sizeof(wchar_t));
	errno_t err = _wcsupr_s(charBuf, s.length() + 1);

	wstring result = charBuf;
	delete charBuf;

	if (err == 0)
		return result;
	else
		return s;
}

wstring StringHelper::trim(const wstring& s)
{
	int firstNonSpace = -1;
	int lastNonSpace = -1;

	for (unsigned i = 0; i < s.length(); i++)
	{
		wchar_t c = s[i];
		if (!iswspace(c))
		{
			if (firstNonSpace == -1)
				firstNonSpace = i;
			lastNonSpace = i;
		}
	}

	if (firstNonSpace == -1)
		return L"";
	else
		return s.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
}

vector<wstring> StringHelper::split(const wstring& s, wchar_t splitChar, bool skipEmpty)
{
	vector<wstring> result;
	size_t prevPos = 0;
	size_t pos = 0;
	while ((pos = s.find(splitChar, pos)) != wstring::npos)
	{
		wstring part = s.substr(prevPos, pos - prevPos);
		if (part.length() > 0 || !skipEmpty)
			result.push_back(part);
		prevPos = ++pos;
	}

	wstring part = s.substr(prevPos);
	if (part.length() > 0 || !skipEmpty)
		result.push_back(part);

	return result;
}

wstring StringHelper::join(const vector<wstring>& strings, const wstring& separator)
{
	wstringstream stream;

	bool first = true;

	for (vector<wstring>::const_iterator it = strings.cbegin(); it != strings.cend(); it++)
	{
		if (first)
			first = false;
		else
			stream << separator;
		stream << *it;
	}

	return stream.str();
}

wstring StringHelper::getSystemErrorString(long status)
{
	wchar_t* buf;

	if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, status, 0, (LPTSTR)&buf, 0, NULL) != 0)
	{
		wstring result(buf);
		LocalFree(buf);

		// remove trailing newline
		if (result.back() == L'\n')
			result.erase(prev(result.end()));
		if (result.back() == L'\r')
			result.erase(prev(result.end()));
		return result;
	}
	else
		return L"";
}

vector<wstring> StringHelper::splitQuoted(const wstring& s, wchar_t splitChar, wchar_t quoteChar)
{
	vector<wstring> result;
	bool inQuotes = false;
	wstring current;
	for (size_t i = 0; i < s.length(); i++)
	{
		wchar_t c = s[i];
		if (c == splitChar && !inQuotes)
		{
			if (current != L"")
			{
				result.push_back(current);
				current = L"";
			}
		}
		else if (c == quoteChar)
		{
			inQuotes = !inQuotes;
			if (inQuotes && i > 0 && s[i - 1] == quoteChar)
				current += quoteChar;
		}
		else
		{
			current += c;
		}
	}

	if (current != L"")
		result.push_back(current);

	return result;
}
