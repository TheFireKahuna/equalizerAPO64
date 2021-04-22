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
#include <regex>

#include "RegexFunctions.h"

using namespace std;
using namespace mup;

RegexSearchFunction::RegexSearchFunction()
	: ICallback(cmFUNC, L"regexSearch", 2)
{
}

void RegexSearchFunction::Eval(ptr_val_type& ret, const ptr_val_type* arg, int argc)
{
	if (!arg[0]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[0]->GetType(), 's', 1));
	if (!arg[1]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[1]->GetType(), 's', 2));

	wstring regexString = arg[0]->GetString();
	wstring string = arg[1]->GetString();

	wregex regex(regexString);
	wsmatch match;
	bool found = regex_search(string, match, regex);

	vector<Value> result;
	if (found)
	{
		for (unsigned i = 0; i < match.size(); i++)
			result.push_back(match.str(i));
	}

	*ret = result;
}

const char_type* RegexSearchFunction::GetDesc() const
{
	return L"regexSearch - searches first match to regular expression. result is empty if not matching";
}

IToken* RegexSearchFunction::Clone() const
{
	return new RegexSearchFunction(*this);
}

RegexReplaceFunction::RegexReplaceFunction()
	: ICallback(cmFUNC, L"regexReplace", 3)
{
}

void RegexReplaceFunction::Eval(ptr_val_type& ret, const ptr_val_type* arg, int argc)
{
	if (!arg[0]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[0]->GetType(), 's', 1));
	if (!arg[1]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[1]->GetType(), 's', 2));
	if (!arg[2]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[2]->GetType(), 's', 3));

	wstring regexString = arg[0]->GetString();
	wstring string = arg[1]->GetString();
	wstring replacement = arg[2]->GetString();

	wregex regex(regexString);
	wstring result = regex_replace(string, regex, replacement);

	*ret = result;
}

const char_type* RegexReplaceFunction::GetDesc() const
{
	return L"regexReplace - replaces all matches to regular expression. result is replaced string";
}

IToken* RegexReplaceFunction::Clone() const
{
	return new RegexReplaceFunction(*this);
}
