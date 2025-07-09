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
#define _USE_MATH_DEFINES
#include <cmath>
#include <regex>
#include <sstream>

#include "helpers/MemoryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "IIRFilter.h"
#include "IIRFilterFactory.h"

using namespace std;

static wregex regexType(L"^\\s*ON\\s+([A-Za-z]+)");
static wregex regexOrder(L"\\s*Order\\s+([0-9]+)");
static wregex regexCoefficients(L"\\s+Coefficients((?: [-+0-9.eE]+)+)");

IIRFilterFactory::IIRFilterFactory()
{
}

vector<IFilter*> IIRFilterFactory::createFilter(const wstring& configPath, wstring& command, wstring& parameters)
{
	IIRFilter* filter = NULL;

	if (command.find(L"Filter") == 0)
	{
		wsmatch match;
		wstring typeString;

		bool found = regex_search(parameters, match, regexType);
		if (found)
		{
			typeString = match.str(1);
			if (typeString == L"IIR")
			{
				bool found = regex_search(parameters, match, regexOrder);
				if (found)
				{
					wstring orderString = match.str(1);
					unsigned order = wcstol(orderString.c_str(), NULL, 10);

					if (order < 1)
					{
						LogF(L"Order %d not supported, must at least be 1", order);
					}
					else
					{
						found = regex_search(parameters, match, regexCoefficients);

						if (found)
						{
							wstring coefficientsString = match.str(1);
							vector<wstring> coefficientStrings = StringHelper::split(coefficientsString, L' ');
							if (coefficientStrings.size() != (order + 1) * 2)
							{
								LogF(L"Invalid number of coefficients. Expected %d coefficients instead of %d", (order + 1) * 2, coefficientStrings.size());
							}
							else
							{
								vector<double> coefficients;
								for (auto it = coefficientStrings.begin(); it != coefficientStrings.end(); it++)
								{
									coefficients.push_back(wcstod(it->c_str(), NULL));
								}

								wstringstream stream;
								stream << L"Adding IIR filter of order " << order << " with coefficients";
								for (unsigned i = 0; i <= order; i++)
									stream << L" b" << i << L"=" << coefficients[i];
								for (unsigned i = 0; i <= order; i++)
									stream << L" a" << i << L"=" << coefficients[i + order + 1];

								TraceF(L"%s", stream.str().c_str());

								void* mem = MemoryHelper::alloc(sizeof(IIRFilter));
								filter = new(mem) IIRFilter(coefficients);
							}
						}
					}
				}
			}
		}
	}

	if (filter == NULL)
		return vector<IFilter*>(0);
	return vector<IFilter*>(1, filter);
}
