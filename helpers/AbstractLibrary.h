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

#pragma once

#include <string>

class AbstractLibrary
{
public:
	static const int FILE_NOT_FOUND = -1;
	static const int LOADING_FAILED = -2;
	static const int FUNCTIONS_MISSING = -3;
	static const int WRONG_ARCHITECTURE = -4;

	virtual ~AbstractLibrary();

	int initialize();
	virtual std::wstring getLibPath() = 0;

protected:
	virtual bool loadFunctions() = 0;
	virtual int customInitialize();

	HMODULE module = NULL;

private:
	unsigned short getFileArchitecture(const std::wstring& filePath);
};
