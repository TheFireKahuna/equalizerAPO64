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
#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "parser/RegexFunctions.h"
#include "parser/RegistryFunctions.h"
#include "FilterEngine.h"
#include "IfFilterFactory.h"

using namespace std;
using namespace mup;

void IfFilterFactory::initialize(FilterEngine* engine)
{
	parser = engine->getParser();
}

vector<IFilter*> IfFilterFactory::startOfConfiguration()
{
	trueCount = 0;
	falseCount = 0;
	while (!trueCountStack.empty())
		trueCountStack.pop();

	return vector<IFilter*>();
}

std::vector<IFilter*> IfFilterFactory::startOfFile(const std::wstring& configPath)
{
	trueCountStack.push(trueCount);
	trueCount = 0;
	executeElse = false;
	if (falseCount != 0)
	{
		LogF(L"File was included inside If that evaluated to false!");
		falseCount = 0;
	}

	return vector<IFilter*>();
}

vector<IFilter*> IfFilterFactory::createFilter(const std::wstring& configPath, std::wstring& command, std::wstring& parameters)
{
	wstring expression = StringHelper::trim(parameters);

	if (command == L"If")
	{
		if (falseCount == 0)
		{
			try
			{
				parser->SetExpr(expression);
				Value result = parser->Eval();
				bool isTrue = toBoolean(result);
				if (result.GetType() == L'b')
					TraceF(L"If(%s) evaluated to %s", expression.c_str(), result.ToString().c_str());
				else
					TraceF(L"If(%s) evaluated to %s (%s)", expression.c_str(), result.ToString().c_str(), isTrue ? L"true" : L"false");

				if (isTrue)
				{
					trueCount++;
				}
				else
				{
					falseCount++;
					executeElse = true;
				}
			}
			catch (ParserError e)
			{
				LogF(L"Error while evaluating If(%s): %s", expression.c_str(), e.GetMsg().c_str());
				falseCount++;
			}
		}
		else
		{
			falseCount++;
		}
	}
	else if (command == L"ElseIf")
	{
		if (falseCount == 0)
		{
			if (trueCount == 0)
			{
				LogF(L"ElseIf without If!");
			}
			else
			{
				falseCount++;
				trueCount--;
			}
		}
		else if (falseCount == 1 && executeElse)
		{
			try
			{
				parser->SetExpr(expression);
				Value result = parser->Eval();
				bool isTrue = toBoolean(result);
				if (result.GetType() == L'b')
					TraceF(L"ElseIf(%s) evaluated to %s", expression.c_str(), result.ToString().c_str());
				else
					TraceF(L"ElseIf(%s) evaluated to %s (%s)", expression.c_str(), result.ToString().c_str(), isTrue ? L"true" : L"false");

				if (isTrue)
				{
					falseCount--;
					trueCount++;
					executeElse = false;
				}
			}
			catch (ParserError e)
			{
				LogF(L"Error while evaluating ElseIf(%s): %s", expression.c_str(), e.GetMsg().c_str());
			}
		}
	}
	else if (command == L"Else")
	{
		if (falseCount == 0)
		{
			if (trueCount == 0)
			{
				LogF(L"Else without If!");
			}
			else
			{
				falseCount++;
				trueCount--;
			}
		}
		else if (falseCount == 1 && executeElse)
		{
			falseCount--;
			trueCount++;
			executeElse = false;
		}
	}
	else if (command == L"EndIf")
	{
		if (falseCount == 0)
		{
			if (trueCount == 0)
				LogF(L"EndIf without If!");
			else
				trueCount--;
		}
		else
		{
			falseCount--;
		}

		if (falseCount == 0)
			executeElse = false;
	}

	if (falseCount > 0)
		// skip line for further factories
		command = L"";

	return vector<IFilter*>();
}

std::vector<IFilter*> IfFilterFactory::endOfFile(const std::wstring& configPath)
{
	if (trueCount != 0 || falseCount != 0)
	{
		LogF(L"If was not closed by EndIf!");
		falseCount = 0;
	}
	trueCount = trueCountStack.top();
	trueCountStack.pop();

	return vector<IFilter*>();
}

bool IfFilterFactory::toBoolean(const Value& value)
{
	bool result = false;

	wchar_t type = value.GetType();
	switch (type)
	{
	case 'i':
		{
			int i = value.GetInteger();
			result = (i != 0);
		}
		break;
	case 'f':
		{
			double f = value.GetFloat();
			result = (f != 0.0 && f == f);
		}
		break;
	case 'b':
		{
			result = value.GetBool();
		}
		break;
	case 's':
		{
			wstring s = value.GetString();
			result = s.length() > 0 && s != L"false" && s != L"0";
		}
		break;
	case 'm':
		{
			Matrix<Value> m = value.GetArray();
			result = m.GetRows() > 0 && m.GetCols() > 0;
		}
		break;
	default:
		result = false;
		break;
	}

	return result;
}
