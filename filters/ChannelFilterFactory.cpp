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
#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "ChannelFilter.h"
#include "ChannelFilterFactory.h"

using namespace std;

vector<IFilter*> ChannelFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	ChannelFilter* filter = NULL;

	if (command == L"Channel")
	{
		wstring value = parameters + L" ";

		vector<wstring> words;
		wstring currentWord;
		for (unsigned i = 0; i < value.length(); i++)
		{
			wchar_t c = towupper(value[i]);

			if (c == L' ')
			{
				if (currentWord.length() > 0)
				{
					words.push_back(currentWord);

					currentWord.clear();
				}
			}
			else
			{
				currentWord += c;
			}
		}

		void* mem = MemoryHelper::alloc(sizeof(ChannelFilter));
		filter = new(mem) ChannelFilter(words);
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}