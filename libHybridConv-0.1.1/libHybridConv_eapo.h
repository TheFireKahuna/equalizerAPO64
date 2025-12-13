/***************************************************************************
 *   Copyright (C) 2009 by Christian Borss                                 *
 *   christian.borss@rub.de                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
// Adapted version for Equalizer APO. For original version see libHybridConv.h

#ifndef __LIBHYBRIDCONV_H__
#define __LIBHYBRIDCONV_H__


#include <fftw3.h>


typedef struct str_HConvSingle
{
	int step;			// processing step counter
	int maxstep;			// number of processing steps per audio frame
	int mixpos;			// current frame index
	int framelength;		// number of samples per audio frame
	int *steptask;			// processing tasks per step
	double *dft_time;		// DFT buffer (time domain)
	fftw_complex *dft_freq;	// DFT buffer (frequency domain)
	double *in_freq_real;		// input buffer (frequency domain)
	double *in_freq_imag;		// input buffer (frequency domain)
	int num_filterbuf;		// number of filter segments
	double **filterbuf_freq_real;	// filter segments (frequency domain)
	double **filterbuf_freq_imag;	// filter segments (frequency domain)
	int num_mixbuf;			// number of mixing segments		
	double **mixbuf_freq_real;	// mixing segments (frequency domain)
	double **mixbuf_freq_imag;	// mixing segments (frequency domain)
	double *history_time;		// history buffer (time domain)
	fftw_plan fft;			// FFT transformation plan
	fftw_plan ifft;		// IFFT transformation plan
} HConvSingle;


typedef struct str_HConvDual
{
	int step;		// processing step counter
	int maxstep;		// number of processing steps per long audio frame
	int flen_long;		// number of samples per long audio frame
	int flen_short;		// number of samples per short audio frame
	double *in_long;		// input buffer (long frame)
	double *out_long;	// output buffer (long frame)
	HConvSingle *f_long;	// convolution filter (long segments)
	HConvSingle *f_short;	// convolution filter (short segments)
} HConvDual;


typedef struct str_HConvTripple
{
	int step;		// processing step counter
	int maxstep;		// number of processing steps per long audio frame
	int flen_medium;	// number of samples per long audio frame
	int flen_short;		// number of samples per short audio frame
	double *in_medium;	// input buffer (long frame)
	double *out_medium;	// output buffer (long frame)
	HConvDual *f_medium;	// convolution filter (long segments)
	HConvSingle *f_short;	// convolution filter (short segments)
} HConvTripple;


/* single filter functions */
double hcTime(void);
double getProcTime(int flen, int num, double dur);
void hcPutSingle(HConvSingle *filter, double*x);
void hcProcessSingle(HConvSingle *filter);
void hcGetSingle(HConvSingle *filter, double*y);
void hcGetAddSingle(HConvSingle *filter, double*y);
void hcInitSingle(HConvSingle *filter, double*h, int hlen, int flen, int steps);
void hcCloseSingle(HConvSingle *filter);

/* dual filter functions */
void hcBenchmarkDual(int sflen, int lflen);
void hcProcessDual(HConvDual *filter, double*in, double*out);
void hcProcessAddDual(HConvDual *filter, double*in, double*out);
void hcInitDual(HConvDual *filter, double*h, int hlen, int sflen, int lflen);
void hcCloseDual(HConvDual *filter);

/* tripple filter functions */
void hcBenchmarkTripple(int sflen, int mflen, int lflen);
void hcProcessTripple(HConvTripple *filter, double*in, double*out);
void hcProcessAddTripple(HConvTripple *filter, double*in, double*out);
void hcInitTripple(HConvTripple *filter, double*h, int hlen, int sflen, int mflen, int lflen);
void hcCloseTripple(HConvTripple *filter);


#endif // __LIBHYBRIDCONV_H__
