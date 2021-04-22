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
#include <sstream>

#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "parser/RegexFunctions.h"
#include "parser/RegistryFunctions.h"
#include "parser/StringOperators.h"
#include "parser/LogicalOperators.h"
#include "FilterEngine.h"
#include "ExpressionFilterFactory.h"

using namespace std;
using namespace mup;

void ExpressionFilterFactory::initialize(FilterEngine* engine)
{
	parser = engine->getParser();
	parser->DefineConst(L"inputChannelCount", (int)engine->getInputChannelCount());
	parser->DefineConst(L"outputChannelCount", (int)engine->getOutputChannelCount());
	parser->DefineConst(L"sampleRate", engine->getSampleRate());

	parser->DefineFun(new ReadRegStringFunction(engine));
	parser->DefineFun(new ReadRegDWORDFunction(engine));
	parser->DefineFun(new RegexSearchFunction());
	parser->DefineFun(new RegexReplaceFunction());

	parser->RemoveOprt(L"+");
	parser->DefineOprt(new AddOperator());
	parser->DefineInfixOprt(new NotOperator());
}

vector<IFilter*> ExpressionFilterFactory::createFilter(const std::wstring& configPath, std::wstring& command, std::wstring& parameters)
{
	if (command.length() > 0 && command[0] == L'#')
		return vector<IFilter*>();

	bool inExpression = false;
	bool lastWasBackslash = false;

	wstring output;
	wstring expression;
	for (unsigned i = 0; i < parameters.size(); i++)
	{
		wchar_t c = parameters[i];
		if (c == L'`')
		{
			if (!inExpression)
			{
				if (lastWasBackslash)
				{
					output += c;
				}
				else
				{
					inExpression = true;
				}
			}
			else
			{
				if (lastWasBackslash)
				{
					expression += c;
				}
				else
				{
					inExpression = false;
					try
					{
						parser->SetExpr(expression);
						Value result = parser->Eval();
						wstring resultString;
						if (result.GetType() == L's')
							resultString = result.GetString();
						else
							resultString = result.ToString().c_str();
						output += resultString;
						TraceF(L"Inline expression %s evaluated to %s", expression.c_str(), resultString.c_str());
					}
					catch (ParserError e)
					{
						LogF(L"Error while evaluating inline expression %s: %s", expression.c_str(), e.GetMsg().c_str());
					}
					expression.clear();
				}
			}
			lastWasBackslash = false;
		}
		else if (c == L'\\')
		{
			if (i >= parameters.size() - 1 || parameters[i + 1] != L'`')
			{
				if (inExpression)
					expression += c;
				else
					output += c;
			}
			lastWasBackslash = true;
		}
		else
		{
			if (inExpression)
				expression += c;
			else
				output += c;
			lastWasBackslash = false;
		}
	}

	parameters = output;

	if (command == L"Eval")
	{
		try
		{
			expression = StringHelper::trim(parameters);
			parser->SetExpr(expression);
			Value result = parser->Eval();
			wstring resultString;
			if (result.GetType() == L's')
				resultString = result.GetString();
			else
				resultString = result.ToString().c_str();
			TraceF(L"Expression %s evaluated to %s", expression.c_str(), resultString.c_str());
		}
		catch (ParserError e)
		{
			LogF(L"Error while evaluating expression %s: %s", expression.c_str(), e.GetMsg().c_str());
		}

		// command has been handled
		command = L"";
	}

	return vector<IFilter*>();
}
