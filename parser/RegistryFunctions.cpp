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
#include "RegistryFunctions.h"
#include "../helpers/RegistryHelper.h"
#include "../FilterEngine.h"

using namespace std;
using namespace mup;

ReadRegStringFunction::ReadRegStringFunction(FilterEngine* engine)
	: ICallback(cmFUNC, L"readRegString", 2)
{
	this->engine = engine;
}

void ReadRegStringFunction::Eval(ptr_val_type& ret, const ptr_val_type* arg, int argc)
{
	if (!arg[0]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[0]->GetType(), 's', 1));
	if (!arg[1]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[1]->GetType(), 's', 2));

	wstring key = arg[0]->GetString();
	wstring valuename = arg[1]->GetString();

	try
	{
		wstring value = RegistryHelper::readValue(key, valuename);

		*ret = value;

		engine->watchRegistryKey(key);
	}
	catch (RegistryException e)
	{
		throw ParserError(e.getMessage());
	}
}

const char_type* ReadRegStringFunction::GetDesc() const
{
	return L"readRegString - read a string value from the registry";
}

IToken* ReadRegStringFunction::Clone() const
{
	return new ReadRegStringFunction(*this);
}

ReadRegDWORDFunction::ReadRegDWORDFunction(FilterEngine* engine)
	: ICallback(cmFUNC, L"readRegDWORD", 2)
{
	this->engine = engine;
}

void ReadRegDWORDFunction::Eval(ptr_val_type& ret, const ptr_val_type* arg, int argc)
{
	if (!arg[0]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[0]->GetType(), 's', 1));
	if (!arg[1]->IsString())
		throw ParserError(ErrorContext(ecTYPE_CONFLICT_FUN, -1, GetIdent(), arg[1]->GetType(), 's', 2));

	wstring key = arg[0]->GetString();
	wstring valuename = arg[1]->GetString();

	try
	{
		*ret = (int) RegistryHelper::readDWORDValue(key, valuename);

		engine->watchRegistryKey(key);
	}
	catch (RegistryException e)
	{
		throw ParserError(e.getMessage());
	}
}

const char_type* ReadRegDWORDFunction::GetDesc() const
{
	return L"readRegDWORD - read a DWORD value from the registry";
}

IToken* ReadRegDWORDFunction::Clone() const
{
	return new ReadRegDWORDFunction(*this);
}
