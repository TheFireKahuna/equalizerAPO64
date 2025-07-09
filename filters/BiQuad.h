/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2013  Jonas Thedering

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

#define _USE_MATH_DEFINES
#include <cmath>
#include <climits>
#include <string>

#define IS_DENORMAL(d) (abs(d) < DBL_MIN)

class BiQuad
{
public:
	enum Type
	{
		LOW_PASS, HIGH_PASS, BAND_PASS, NOTCH, ALL_PASS, PEAKING, LOW_SHELF, HIGH_SHELF
	};

	BiQuad() {}
	BiQuad(Type type, double dbGain, double freq, double srate, double bandwidthOrQOrS, bool isBandwidthOrS);

	__forceinline
	void removeDenormals()
	{
		if (IS_DENORMAL(x1))
			x1 = 0.0;
		if (IS_DENORMAL(x2))
			x2 = 0.0;
		if (IS_DENORMAL(y1))
			y1 = 0.0;
		if (IS_DENORMAL(y2))
			y2 = 0.0;
	}

	__forceinline
	double process(double sample)
	{
		// changed order of additions leads to better pipelining
		double result = a0 * sample + a[1] * x2 + a[0] * x1 - a[3] * y2 - a[2] * y1;

		x2 = x1;
		x1 = sample;

		y2 = y1;
		y1 = result;

		return result;
	}

	__forceinline
	void setCoefficients(double ain[], const double& a0in)
	{
		for (int i = 0; i < 4; i++)
			a[i] = ain[i];
		a0 = a0in;
	}

	double gainAt(double freq, double srate);
	void getCoefficients(double(&out_coeffs)[4], double& out_a0) const;

private:
	__declspec(align(16)) double a[4];
	double a0;

	double x1, x2;
	double y1, y2;
};
