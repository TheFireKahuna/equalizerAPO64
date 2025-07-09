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

#pragma once

#include <mpParser.h>

class FilterEngine;

class ReadRegStringFunction : public mup::ICallback
{
public:
	ReadRegStringFunction(FilterEngine* engine);
	void Eval(mup::ptr_val_type& ret, const mup::ptr_val_type* arg, int argc) override;
	const mup::char_type* GetDesc() const override;
	mup::IToken* Clone() const override;

private:
	FilterEngine* engine;
};

class ReadRegDWORDFunction : public mup::ICallback
{
public:
	ReadRegDWORDFunction(FilterEngine* engine);
	void Eval(mup::ptr_val_type& ret, const mup::ptr_val_type* arg, int argc) override;
	const mup::char_type* GetDesc() const override;
	mup::IToken* Clone() const override;

private:
	FilterEngine* engine;
};
