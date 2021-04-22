/***************************************************************************
 *   Copyright (C) 2009 by Christian Borss                                 *
 *   christian.borss@rub.de                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "libHybridConv.h"
#include <math.h>


int main(void)
{
	int s, m, l;
	int sflen, mflen, lflen;
	int num;
	double tau_1, tau_16;
	double cpu_load;
	double c0[9];
	double c1[9];
	double tau_s, tau_m, tau_l;
	int num_s, num_m, num_l;
	int hlen = 96000;
	int f_s = 48000;

	// performance measurement with uniform segmentation
	for (s = 0; s < 9; s++)
	{
		sflen = 64 << s;
		num = 1;
		printf("%5d / %2d    ", sflen, num);
		tau_1 = getProcTime(sflen, num, 2.0);
		num = 16;
		printf("%5d / %2d    ", sflen, num);
		tau_16 = getProcTime(sflen, num, 2.0);
		c1[s] = (tau_16 - tau_1) / 15.0;
		c0[s] = tau_1 - c1[s];
		printf("\n");
	}
	printf("\n");

	// performance prediction with 3 segment lengths
	for (s = 0; s < 5; s++)
	{
		for (m = 1; m < 5; m++)
		{
			for (l = 1; l+m+s < 9; l++)
			{
				sflen = 64 << s;
				mflen = sflen << m;
				lflen = mflen << l;

				num_s = mflen / sflen;
				num_m = 2 * lflen / mflen;
				num_l = ceil((hlen - num_s * sflen - num_m * mflen) / (double)lflen);
				tau_s = c0[s]     + c1[s]     * num_s;
				tau_m = c0[s+m]   + c1[s+m]   * num_m;
				tau_l = c0[s+m+l] + c1[s+m+l] * num_l;

				cpu_load = 100.0 * (tau_s * lflen / sflen + tau_m * lflen / mflen + tau_l) * f_s / (double)lflen;

				printf("%4d / %4d / %4d    ", sflen, mflen, lflen);
				printf("Predicted CPU load: %5.2f %%\n", cpu_load);
			}
		}
		printf("\n");
	}
	printf("\n");

	// performance prediction with 2 segment lengths
	for (m = 0; m < 8; m++)
	{
		for (l = 1; l+m < 9; l++)
		{
			mflen = 64 << m;
			lflen = mflen << l;

			num_m = 2 * lflen / mflen;
			num_l = ceil((hlen - num_m * mflen) / (double)lflen);
			tau_m = c0[m]   + c1[m]   * num_m;
			tau_l = c0[m+l] + c1[m+l] * num_l;

			cpu_load = 100.0 * (tau_m * lflen / mflen + tau_l) * f_s / (double)lflen;

			printf("     / %4d / %4d    ", mflen, lflen);
			printf("Predicted CPU load: %5.2f %%\n", cpu_load);
		}
		printf("\n");
	}
	printf("\n");

	// performance prediction with 1 segment lengths
	for (l = 0; l < 9; l++)
	{
		lflen = 64 << l;

		num_l = ceil(hlen / (double)lflen);
		tau_l = c0[l] + c1[l] * num_l;

		cpu_load = 100.0 * tau_l * f_s / (double)lflen;

		printf("     /      / %4d    ", lflen);
		printf("Predicted CPU load: %5.2f %%\n", cpu_load);
	}
	printf("\n");

	for (s = 0; s < 5; s++)
	{
		for (m = 1; m < 5; m++)
		{
			for (l = 1; l+m+s < 9; l++)
			{
				sflen = 64 << s;
				mflen = sflen << m;
				lflen = mflen << l;
				printf("%4d / %4d / %4d    ", sflen, mflen, lflen);
				hcBenchmarkTripple(sflen, mflen, lflen);
			}
		}
		printf("\n");
	}

	return 0;
}
