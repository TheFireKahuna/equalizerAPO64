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
#include "CopyFilter.h"
#include "CopyFilterFactory.h"

using namespace std;

vector<IFilter*> CopyFilterFactory::createFilter(const std::wstring& configPath, std::wstring& command, std::wstring& parameters)
{
	CopyFilter* filter = NULL;

	if (command == L"Copy")
	{
		vector<Assignment> assignments;

		vector<wstring> assignmentStrings = StringHelper::split(parameters, L' ');
		for (vector<wstring>::iterator it = assignmentStrings.begin(); it != assignmentStrings.end(); it++)
		{
			Assignment assignment;

			vector<wstring> parts = StringHelper::split(*it, L'=');
			if (parts.size() == 2)
			{
				wstring target = parts[0];
				wstring source = parts[1];

				assignment.targetChannel = target;

				vector<wstring> summands = StringHelper::split(source, '+');
				for (vector<wstring>::iterator it2 = summands.begin(); it2 != summands.end(); it2++)
				{
					vector<wstring> factors = StringHelper::split(*it2, '*');
					wstring factor;
					wstring channel;
					if (factors.size() == 2)
					{
						factor = factors[0];
						channel = factors[1];
					}
					else if (factors.size() == 1)
					{
						if (factors[0] == L"0" || factors[0].find(L'.') != wstring::npos)
							factor = factors[0];
						else
							channel = factors[0];
					}

					Assignment::Summand summand;
					if (factor == L"")
					{
						summand.factor = 1.0;
						summand.isDecibel = false;
					}
					else
					{
						summand.factor = wcstod(factor.c_str(), NULL);
						summand.isDecibel = factor.size() > 2 && StringHelper::toLowerCase(factor.substr(factor.size() - 2)) == L"db";
					}

					summand.channel = channel;
					assignment.sourceSum.push_back(summand);
				}
			}

			if (assignment.targetChannel != L"" && !assignment.sourceSum.empty())
				assignments.push_back(assignment);
		}

		void* mem = MemoryHelper::alloc(sizeof(CopyFilter));
		filter = new(mem) CopyFilter(assignments);
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}