/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

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

#include <cmath>
#include "ExponentialSpinBox.h"

using namespace std;

ExponentialSpinBox::ExponentialSpinBox(QWidget* parent)
	: QSpinBox(parent)
{
}

void ExponentialSpinBox::stepBy(int steps)
{
	qint64 v = value();
	if (steps > 0)
	{
		v <<= steps;
		if (v > maximum())
			v = maximum();
	}
	else
	{
		v >>= -steps;
		if (v < minimum())
			v = minimum();
	}

	setValue((int)v);
}

int ExponentialSpinBox::valueFromText(const QString& text) const
{
	int value = QSpinBox::valueFromText(text);
	value = exp2(round(log2(value)));
	return value;
}
