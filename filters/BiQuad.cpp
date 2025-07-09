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

#include "stdafx.h"
#include "BiQuad.h"

using namespace std;

BiQuad::BiQuad(Type type, double dbGain, double freq, double srate, double bandwidthOrQOrS, bool isBandwidthOrS)
{
	double A;
	if (type == PEAKING || type == LOW_SHELF || type == HIGH_SHELF)
		A = pow(10, dbGain / 40);
	else
		A = pow(10, dbGain / 20);
	double omega = 2 * M_PI * freq / srate;
	double sn = sin(omega);
	double cs = cos(omega);
	double alpha;

	if (!isBandwidthOrS) // Q
		alpha = sn / (2 * bandwidthOrQOrS);
	else if (type == LOW_SHELF || type == HIGH_SHELF) // S
		alpha = sn / 2 * sqrt((A + 1 / A) * (1 / bandwidthOrQOrS - 1) + 2);
	else // BW
		alpha = sn * sinh(M_LN2 / 2 * bandwidthOrQOrS * omega / sn);

	double beta = 2 * sqrt(A) * alpha;

	double b0, b1, b2, a0, a1, a2;

	switch (type)
	{
	case LOW_PASS:
		b0 = (1 - cs) / 2;
		b1 = 1 - cs;
		b2 = (1 - cs) / 2;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case HIGH_PASS:
		b0 = (1 + cs) / 2;
		b1 = -(1 + cs);
		b2 = (1 + cs) / 2;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BAND_PASS:
		b0 = alpha;
		b1 = 0;
		b2 = -alpha;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case NOTCH:
		b0 = 1;
		b1 = -2 * cs;
		b2 = 1;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case ALL_PASS:
		b0 = 1 - alpha;
		b1 = -2 * cs;
		b2 = 1 + alpha;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case PEAKING:
		b0 = 1 + (alpha * A);
		b1 = -2 * cs;
		b2 = 1 - (alpha * A);
		a0 = 1 + (alpha / A);
		a1 = -2 * cs;
		a2 = 1 - (alpha / A);
		break;
	case LOW_SHELF:
		b0 = A * ((A + 1) - (A - 1) * cs + beta);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta);
		a0 = (A + 1) + (A - 1) * cs + beta;
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta;
		break;
	case HIGH_SHELF:
		b0 = A * ((A + 1) + (A - 1) * cs + beta);
		b1 = -2 * A * ((A - 1) + (A + 1) * cs);
		b2 = A * ((A + 1) + (A - 1) * cs - beta);
		a0 = (A + 1) - (A - 1) * cs + beta;
		a1 = 2 * ((A - 1) - (A + 1) * cs);
		a2 = (A + 1) - (A - 1) * cs - beta;
		break;
	}

	this->a0 = b0 / a0;
	this->a[0] = b1 / a0;
	this->a[1] = b2 / a0;
	this->a[2] = a1 / a0;
	this->a[3] = a2 / a0;

	x1 = 0;
	x2 = 0;
	y1 = 0;
	y2 = 0;
}

double BiQuad::gainAt(double freq, double srate)
{
	double omega = 2 * M_PI * freq / srate;
	double sn = sin(omega / 2.0);
	double phi = sn * sn;
	double b0 = this->a0;
	double b1 = this->a[0];
	double b2 = this->a[1];
	double a0 = 1.0;
	double a1 = this->a[2];
	double a2 = this->a[3];

	double dbGain = 10 * log10(pow(b0 + b1 + b2, 2) - 4 * (b0 * b1 + 4 * b0 * b2 + b1 * b2) * phi + 16 * b0 * b2 * phi * phi)
		- 10 * log10(pow(a0 + a1 + a2, 2) - 4 * (a0 * a1 + 4 * a0 * a2 + a1 * a2) * phi + 16 * a0 * a2 * phi * phi);

	return dbGain;
}

void BiQuad::getCoefficients(double(&out_coeffs)[4], double& out_a0) const
{
	out_a0 = this->a0;
	for (int i = 0; i < 4; ++i)
	{
		out_coeffs[i] = this->a[i];
	}
}