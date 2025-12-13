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
 // Adapted version for Equalizer APO. For original version see libHybridConv.c

#include "stdafx.h"
#ifndef _M_ARM64
#include <immintrin.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif
#include <math.h>
#include <fftw3.h>
#include "libHybridConv_eapo.h"


double hcTime(void)
{
#ifdef WIN32
	ULONGLONG t = GetTickCount64();
	return (double)t * 0.001;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 0.000001;
#endif
}

////////////////////////////////////////////////////////////////

double getProcTime(int flen, int num, double dur)
{
	HConvSingle filter;
	double* x;
	double* h;
	double* y;
	int xlen, hlen, ylen;
	int size, n;
	int pos;
	double t_start, t_diff;
	double counter = 0.0;
	double proc_time;
	double lin, mul;

	xlen = 2048 * 2048;
	size = sizeof(double) * xlen;
	x = (double*)fftw_malloc(size);
	lin = pow(10.0, -100.0 / 20.0);	// 0.00001 = -100dB
	mul = pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (n = 1; n < xlen; n++)
		x[n] = (double)(-mul * x[n - 1]);

	hlen = flen * num;
	size = sizeof(double) * hlen;
	h = (double*)fftw_malloc(size);
	lin = pow(10.0, -60.0 / 20.0);	// 0.001 = -60dB
	mul = pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (n = 1; n < hlen; n++)
		h[n] = (double)(mul * h[n - 1]);

	ylen = flen;
	size = sizeof(double) * ylen;
	y = (double*)fftw_malloc(size);

	hcInitSingle(&filter, h, hlen, flen, 1);

	t_diff = 0.0;
	t_start = hcTime();
	pos = 0;
	while (t_diff < dur)
	{
		hcPutSingle(&filter, &x[pos]);
		hcProcessSingle(&filter);
		hcGetSingle(&filter, y);
		pos += flen;
		if (pos >= xlen)
			pos = 0;
		counter += 1.0;
		t_diff = hcTime() - t_start;
	}
	proc_time = t_diff / counter;
	printf("Processing time: %7.3f us\n", 1000000.0 * proc_time);

	hcCloseSingle(&filter);
	fftw_free(x);
	fftw_free(h);
	fftw_free(y);

	return proc_time;
}

void hcPutSingle(HConvSingle* filter, double* x)
{
	const size_t flen = (size_t)filter->framelength;
	const size_t dft_len = 2 * flen;
	const size_t freq_len = flen + 1;

	// --- Phase 1: Input Preparation (copy x[0..flen-1], zero-pad [flen..2*flen-1]) ---
	size_t n = 0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
	{
		const size_t simd_width = 8; // doubles
		const __m512d zero_vec = _mm512_setzero_pd();

		for (; n + simd_width <= flen; n += simd_width) {
			__m512d v = _mm512_loadu_pd(x + n);
			_mm512_storeu_pd(filter->dft_time + n, v);
		}
		// Only zero-pad if we've finished copying input
		if (n >= flen) {
			for (; n + simd_width <= dft_len; n += simd_width) {
				_mm512_storeu_pd(filter->dft_time + n, zero_vec);
			}
		}
	}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
	{
		const size_t simd_width = 4;
		const __m256d zero_vec = _mm256_setzero_pd();

		for (; n + simd_width <= flen; n += simd_width) {
			__m256d v = _mm256_loadu_pd(x + n);
			_mm256_storeu_pd(filter->dft_time + n, v);
		}
		// Only zero-pad if we've finished copying input
		if (n >= flen) {
			for (; n + simd_width <= dft_len; n += simd_width) {
				_mm256_storeu_pd(filter->dft_time + n, zero_vec);
			}
		}
	}
#endif

#if !defined(_M_ARM64)
	{
		const size_t simd_width = 2;
		const __m128d zero_vec = _mm_setzero_pd();

		for (; n + simd_width <= flen; n += simd_width) {
			__m128d v = _mm_loadu_pd(x + n);
			_mm_storeu_pd(filter->dft_time + n, v);
		}
		// Only zero-pad if we've finished copying input
		if (n >= flen) {
			for (; n + simd_width <= dft_len; n += simd_width) {
				_mm_storeu_pd(filter->dft_time + n, zero_vec);
			}
		}
	}
#endif

	if (n < flen) {
		memcpy(filter->dft_time + n, x + n, (flen - n) * sizeof(double));
		n = flen;
	}
	if (n < dft_len) {
		memset(filter->dft_time + n, 0, (dft_len - n) * sizeof(double));
	}

	// --- Phase 2: FFT ---
	fftw_execute(filter->fft);

	// --- Phase 3: De-interleave FFTW complex output into planar real/imag ---
	size_t j = 0;
	fftw_complex* dft_freq = filter->dft_freq;
	
#if defined(__AVX512F__) && !defined(_M_ARM64)
    {
        // Process 8 complex numbers per loop (r0 i0 ... r7 i7)
        const size_t CW = 8;

        // Indices for _mm512_permutex2var_pd:
        //   0..7  select from 'a' (first source)
        //   8..15 select from 'b' (second source)
        const __m512i idx_real = _mm512_setr_epi64(
            0, 2, 4, 6,   // r0 r1 r2 r3 from 'a'
            8 + 0, 8 + 2, 8 + 4, 8 + 6  // r4 r5 r6 r7 from 'b'
        );
        const __m512i idx_imag = _mm512_setr_epi64(
            1, 3, 5, 7,   // i0 i1 i2 i3 from 'a'
            8 + 1, 8 + 3, 8 + 5, 8 + 7  // i4 i5 i6 i7 from 'b'
        );

        for (; j + CW <= freq_len; j += CW) {
            // a = [r0 i0 r1 i1 r2 i2 r3 i3]
            // b = [r4 i4 r5 i5 r6 i6 r7 i7]
            __m512d a = _mm512_loadu_pd((const double*)(dft_freq + j));
            __m512d b = _mm512_loadu_pd((const double*)(dft_freq + j + 4));

            // Directly select real and imag lanes from a|b
            __m512d r = _mm512_permutex2var_pd(a, idx_real, b); // [r0..r7]
            __m512d i = _mm512_permutex2var_pd(a, idx_imag, b); // [i0..i7]

            _mm512_storeu_pd(filter->in_freq_real + j, r);
            _mm512_storeu_pd(filter->in_freq_imag + j, i);
        }
    }
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
	{
		// Process 4 complex numbers (8 doubles) per loop.
		const size_t complex_width = 4;
		for (; j + complex_width <= freq_len; j += complex_width) {
			// c0 = [r0 i0 r1 i1], c1 = [r2 i2 r3 i3]
			__m256d c0 = _mm256_loadu_pd((const double*)(dft_freq + j));
			__m256d c1 = _mm256_loadu_pd((const double*)(dft_freq + j + 2));

			// Interleave within pairs across c0/c1.
			// rtmp = [r0 r2 r1 r3], itmp = [i0 i2 i1 i3]
			__m256d rtmp = _mm256_shuffle_pd(c0, c1, 0x0);
			__m256d itmp = _mm256_shuffle_pd(c0, c1, 0xF);

			// Fix the middle-lane order: [r0 r2 r1 r3] -> [r0 r1 r2 r3]
			// 0xD8 = 11 01 10 00b selects [0,2,1,3].
			__m256d real_vec = _mm256_permute4x64_pd(rtmp, 0xD8);
			__m256d imag_vec = _mm256_permute4x64_pd(itmp, 0xD8);

			_mm256_storeu_pd(filter->in_freq_real + j, real_vec);
			_mm256_storeu_pd(filter->in_freq_imag + j, imag_vec);
		}
	}
#endif

#if !defined(_M_ARM64)
	{
		// Process 2 complex numbers per loop (SSE2).
		const size_t complex_width = 2;
		for (; j + complex_width <= freq_len; j += complex_width) {
			// c0 = [r0 i0], c1 = [r1 i1]
			__m128d c0 = _mm_loadu_pd((const double*)(dft_freq + j));
			__m128d c1 = _mm_loadu_pd((const double*)(dft_freq + j + 1));

			__m128d real_vec = _mm_shuffle_pd(c0, c1, 0x00); // [r0 r1]
			__m128d imag_vec = _mm_shuffle_pd(c0, c1, 0x03); // [i0 i1]

			_mm_storeu_pd(filter->in_freq_real + j, real_vec);
			_mm_storeu_pd(filter->in_freq_imag + j, imag_vec);
		}
	}
#endif

	// Scalar tail (<1 complex for r2c)
	for (; j < freq_len; ++j) {
		filter->in_freq_real[j] = dft_freq[j][0];
		filter->in_freq_imag[j] = dft_freq[j][1];
	}
}


void hcProcessSingle(HConvSingle* filter)
{
	const int flen = filter->framelength;
	// Arrays hold flen+1 doubles (DC..Nyquist)
	const size_t num_elements = (size_t)flen + 1;

	const double* const x_real = filter->in_freq_real;
	const double* const x_imag = filter->in_freq_imag;

	const int start = filter->steptask[filter->step];
	const int stop = filter->steptask[filter->step + 1];

	for (int s = start; s < stop; ++s) {
		const int mix_idx = (s + filter->mixpos) % filter->num_mixbuf;

		double* const       y_real = filter->mixbuf_freq_real[mix_idx];
		double* const       y_imag = filter->mixbuf_freq_imag[mix_idx];
		const double* const h_real = filter->filterbuf_freq_real[s];
		const double* const h_imag = filter->filterbuf_freq_imag[s];

#if !defined(_M_ARM64)
		// Prefetch next filter segment to help hide memory latency.
		if (s + 1 < stop) {
			_mm_prefetch((char const*)(filter->filterbuf_freq_real[s + 1]), _MM_HINT_T0);
			_mm_prefetch((char const*)(filter->filterbuf_freq_imag[s + 1]), _MM_HINT_T0);
		}
#endif

		size_t n = 0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
		// AVX-512: 8 doubles at a time.
		for (; n + 8 <= num_elements; n += 8) {
			const __m512d xr = _mm512_loadu_pd(x_real + n);
			const __m512d xi = _mm512_loadu_pd(x_imag + n);
			const __m512d hr = _mm512_loadu_pd(h_real + n);
			const __m512d hi = _mm512_loadu_pd(h_imag + n);

			__m512d yr = _mm512_loadu_pd(y_real + n);
			__m512d yi = _mm512_loadu_pd(y_imag + n);

			// Real: yr += xr*hr - xi*hi
			yr = _mm512_fmadd_pd(xr, hr, yr);    // yr = xr*hr + yr
			yr = _mm512_fnmadd_pd(xi, hi, yr);   // yr = -(xi*hi) + yr = yr - xi*hi

			// Imag: yi += xr*hi + xi*hr
			yi = _mm512_fmadd_pd(xr, hi, yi);    // yi = xr*hi + yi
			yi = _mm512_fmadd_pd(xi, hr, yi);    // yi = xi*hr + yi

			_mm512_storeu_pd(y_real + n, yr);
			_mm512_storeu_pd(y_imag + n, yi);
		}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
		// AVX2: 4 doubles at a time.
		for (; n + 4 <= num_elements; n += 4) {
			const __m256d xr = _mm256_loadu_pd(x_real + n);
			const __m256d xi = _mm256_loadu_pd(x_imag + n);
			const __m256d hr = _mm256_loadu_pd(h_real + n);
			const __m256d hi = _mm256_loadu_pd(h_imag + n);

			__m256d yr = _mm256_loadu_pd(y_real + n);
			__m256d yi = _mm256_loadu_pd(y_imag + n);

			// Real: yr += xr*hr - xi*hi
			yr = _mm256_fmadd_pd(xr, hr, yr);    // yr = xr*hr + yr
			yr = _mm256_fnmadd_pd(xi, hi, yr);   // yr = -(xi*hi) + yr = yr - xi*hi

			// Imag: yi += xr*hi + xi*hr
			yi = _mm256_fmadd_pd(xr, hi, yi);    // yi = xr*hi + yi
			yi = _mm256_fmadd_pd(xi, hr, yi);    // yi = xi*hr + yi

			_mm256_storeu_pd(y_real + n, yr);
			_mm256_storeu_pd(y_imag + n, yi);
		}
#endif

#if !defined(_M_ARM64)
		// SSE2: 2 doubles at a time.
		for (; n + 2 <= num_elements; n += 2) {
			const __m128d xr = _mm_loadu_pd(x_real + n);
			const __m128d xi = _mm_loadu_pd(x_imag + n);
			const __m128d hr = _mm_loadu_pd(h_real + n);
			const __m128d hi = _mm_loadu_pd(h_imag + n);

			__m128d yr = _mm_loadu_pd(y_real + n);
			__m128d yi = _mm_loadu_pd(y_imag + n);

			// Real: yr += xr*hr - xi*hi
			yr = _mm_fmadd_pd(xr, hr, yr);       // yr = xr*hr + yr
			yr = _mm_fnmadd_pd(xi, hi, yr);      // yr = -(xi*hi) + yr = yr - xi*hi

			// Imag: yi += xr*hi + xi*hr
			yi = _mm_fmadd_pd(xr, hi, yi);       // yi = xr*hi + yi
			yi = _mm_fmadd_pd(xi, hr, yi);       // yi = xi*hr + yi

			_mm_storeu_pd(y_real + n, yr);
			_mm_storeu_pd(y_imag + n, yi);
		}
#endif

		// Scalar tail (and works for ARM64 too).
		for (; n < num_elements; ++n) {
			y_real[n] += x_real[n] * h_real[n] - x_imag[n] * h_imag[n];
			y_imag[n] += x_real[n] * h_imag[n] + x_imag[n] * h_real[n];
		}
	}

	filter->step = (filter->step + 1) % filter->maxstep;
}

static inline void zero_doubles_simd(double* __restrict p, int len)
{
#if defined(__AVX512F__)
	const int VW = 8; // doubles per 512-bit reg
	__m512d z = _mm512_setzero_pd();
	int i = 0;
	for (; i + VW <= len; i += VW) {
		_mm512_storeu_pd(p + i, z);
	}
	for (; i < len; ++i) p[i] = 0.0;
#elif defined(__AVX2__)
	const int VW = 4; // doubles per 256-bit reg
	__m256d z = _mm256_setzero_pd();
	int i = 0;
	for (; i + VW <= len; i += VW) {
		_mm256_storeu_pd(p + i, z);
	}
	for (; i < len; ++i) p[i] = 0.0;
#elif defined(__AVX__)
	const int VW = 4; // AVX1 still has 256-bit double ops
	__m256d z = _mm256_setzero_pd();
	int i = 0;
	for (; i + VW <= len; i += VW) {
		_mm256_storeu_pd(p + i, z);
	}
	for (; i < len; ++i) p[i] = 0.0;
#else
	for (int i = 0; i < len; ++i) {
		p[i] = 0.0;
	}
#endif
}

static inline void add_out_hist_to_y_simd(const double* __restrict out,
	const double* __restrict hist,
	double* __restrict y,
	int len,
	int add_to_existing_y /*0: assign; 1: += */)
{
#if defined(__AVX512F__)
	const int VW = 8;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m512d vout = _mm512_loadu_pd(out + i);
		__m512d vhist = _mm512_loadu_pd(hist + i);
		__m512d vsum = _mm512_add_pd(vout, vhist);
		if (add_to_existing_y) {
			__m512d vy = _mm512_loadu_pd(y + i);
			vsum = _mm512_add_pd(vsum, vy);
		}
		_mm512_storeu_pd(y + i, vsum);
	}
	for (; i < len; ++i) {
		double s = out[i] + hist[i];
		y[i] = add_to_existing_y ? (y[i] + s) : s;
	}
#elif defined(__AVX2__)
	const int VW = 4;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m256d vout = _mm256_loadu_pd(out + i);
		__m256d vhist = _mm256_loadu_pd(hist + i);
		__m256d vsum = _mm256_add_pd(vout, vhist);
		if (add_to_existing_y) {
			__m256d vy = _mm256_loadu_pd(y + i);
			vsum = _mm256_add_pd(vsum, vy);
		}
		_mm256_storeu_pd(y + i, vsum);
	}
	for (; i < len; ++i) {
		double s = out[i] + hist[i];
		y[i] = add_to_existing_y ? (y[i] + s) : s;
	}
#elif defined(__AVX__)
	const int VW = 4;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m256d vout = _mm256_loadu_pd(out + i);
		__m256d vhist = _mm256_loadu_pd(hist + i);
		__m256d vsum = _mm256_add_pd(vout, vhist);
		if (add_to_existing_y) {
			__m256d vy = _mm256_loadu_pd(y + i);
			vsum = _mm256_add_pd(vsum, vy);
		}
		_mm256_storeu_pd(y + i, vsum);
	}
	for (; i < len; ++i) {
		double s = out[i] + hist[i];
		y[i] = add_to_existing_y ? (y[i] + s) : s;
	}
#else
	for (int i = 0; i < len; ++i) {
		double s = out[i] + hist[i];
		y[i] = add_to_existing_y ? (y[i] + s) : s;
	}
#endif
}

static inline void copy_hist_from_out_tail_simd(double* __restrict hist,
	const double* __restrict out_tail,
	int len)
{
#if defined(__AVX512F__)
	const int VW = 8;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m512d v = _mm512_loadu_pd(out_tail + i);
		_mm512_storeu_pd(hist + i, v);
	}
	for (; i < len; ++i) hist[i] = out_tail[i];
#elif defined(__AVX2__)
	const int VW = 4;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m256d v = _mm256_loadu_pd(out_tail + i);
		_mm256_storeu_pd(hist + i, v);
	}
	for (; i < len; ++i) hist[i] = out_tail[i];
#elif defined(__AVX__)
	const int VW = 4;
	int i = 0;
	for (; i + VW <= len; i += VW) {
		__m256d v = _mm256_loadu_pd(out_tail + i);
		_mm256_storeu_pd(hist + i, v);
	}
	for (; i < len; ++i) hist[i] = out_tail[i];
#else
	for (int i = 0; i < len; ++i)
		hist[i] = out_tail[i];
#endif
}

void hcGetSingle(HConvSingle* filter, double* y)
{
	int flen = filter->framelength;
	int mpos = filter->mixpos;

	double* out = filter->dft_time;        // length = 2*flen
	double* hist = filter->history_time;    // length = flen

	// Move one frequency frame from mixbuf -> dft_freq and zero the source.
	// Keep scalar here to preserve exact per-bin assignment order into AoS fftw_complex.
	for (int j = 0; j < flen + 1; ++j)
	{
		filter->dft_freq[j][0] = filter->mixbuf_freq_real[mpos][j];
		filter->dft_freq[j][1] = filter->mixbuf_freq_imag[mpos][j];
	}

	// Zero the mix buffers for this slot (vectorized).
	zero_doubles_simd(filter->mixbuf_freq_real[mpos], flen + 1);
	zero_doubles_simd(filter->mixbuf_freq_imag[mpos], flen + 1);

	// IFFT (unchanged).
	fftw_execute(filter->ifft);

	// Time-domain overlap-add: y[n] = out[n] + hist[n]   (vectorized).
	add_out_hist_to_y_simd(/*out:*/ out,
		/*hist:*/ hist,
		/*y:*/ y,
		/*len:*/ flen,
		/*add_to_existing_y:*/ 0);

	// Update history with tail: hist <- out[flen .. 2*flen-1] (vectorized).
	copy_hist_from_out_tail_simd(hist, out + flen, flen);

	// Advance circular position.
	filter->mixpos = (mpos + 1) % filter->num_mixbuf;
}

void hcGetAddSingle(HConvSingle* filter, double* y)
{
	int flen = filter->framelength;
	int mpos = filter->mixpos;

	double* out = filter->dft_time;        // length = 2*flen
	double* hist = filter->history_time;    // length = flen

	// Move one frequency frame from mixbuf -> dft_freq and zero the source.
	for (int j = 0; j < flen + 1; ++j)
	{
		filter->dft_freq[j][0] = filter->mixbuf_freq_real[mpos][j];
		filter->dft_freq[j][1] = filter->mixbuf_freq_imag[mpos][j];
	}

	zero_doubles_simd(filter->mixbuf_freq_real[mpos], flen + 1);
	zero_doubles_simd(filter->mixbuf_freq_imag[mpos], flen + 1);

	fftw_execute(filter->ifft);

	// Accumulate: y[n] += out[n] + hist[n]   (vectorized).
	add_out_hist_to_y_simd(/*out:*/ out,
		/*hist:*/ hist,
		/*y:*/ y,
		/*len:*/ flen,
		/*add_to_existing_y:*/ 1);

	// Update history with tail.
	copy_hist_from_out_tail_simd(hist, out + flen, flen);

	filter->mixpos = (mpos + 1) % filter->num_mixbuf;
}

static inline void mul_store_gain_double(double* __restrict dst,
	const double* __restrict src,
	int n, double gain)
{
#if defined(__AVX512F__)
	const int V = 8;
	__m512d g = _mm512_set1_pd(gain);
	int i = 0;
	for (; i + V <= n; i += V) {
		__m512d x = _mm512_loadu_pd(src + i);
		x = _mm512_mul_pd(x, g);
		_mm512_storeu_pd(dst + i, x);
	}
	for (; i < n; ++i) dst[i] = src[i] * gain;
#elif defined(__AVX2__) || defined(__AVX__)
	// For doubles, AVX intrinsics (_mm256_*) are the FP workhorses; AVX2 adds ints.
	const int V = 4;
	__m256d g = _mm256_set1_pd(gain);
	int i = 0;
	for (; i + V <= n; i += V) {
		__m256d x = _mm256_loadu_pd(src + i);
		x = _mm256_mul_pd(x, g);
		_mm256_storeu_pd(dst + i, x);
	}
	for (; i < n; ++i) dst[i] = src[i] * gain;
#else
	for (int i = 0; i < n; ++i) dst[i] = src[i] * gain;
#endif
}

static inline void copy_split_complex_scalar(const fftw_complex * __restrict src,
	double* __restrict re,
	double* __restrict im,
	int n_complex)
{
	// n_complex = flen + 1
	for (int j = 0; j < n_complex; ++j) {
		re[j] = src[j][0];
		im[j] = src[j][1];
	}
}

// Vectorized interleaved (re,im) -> planar (re[] / im[])
static inline void copy_split_complex_vec(const fftw_complex* __restrict src,
	double* __restrict re,
	double* __restrict im,
	int n_complex)
{
	const double* s = (const double*)src;
	int j = 0;

#if defined(__AVX512F__)
	// 4 complex per iter: load 8 doubles: [r0,i0,r1,i1,r2,i2,r3,i3]
	for (; j + 4 <= n_complex; j += 4) {
		__m512d v = _mm512_loadu_pd(s + (size_t)j * 2);

		// Build index vectors to select even (re) and odd (im) lanes.
		// We’ll take the lower 256 bits (first 4 lanes) after permute.
		const __m512i idx_even = _mm512_set_epi64(6, 4, 2, 0, 6, 4, 2, 0);
		const __m512i idx_odd = _mm512_set_epi64(7, 5, 3, 1, 7, 5, 3, 1);

		__m512d v_re = _mm512_permutexvar_pd(idx_even, v); // [r0,r2,r4?,r6? | dup]
		__m512d v_im = _mm512_permutexvar_pd(idx_odd, v); // [i0,i2,i4?,i6? | dup]

		// Store lower 256 containing [r0, r1, r2, r3]? Wait—our even/odd
		// indices were 0,2,4,6 and 1,3,5,7. For 4 complexes:
		// even -> [r0,r1,r2,r3] map is [0,2,4,6] over [r0,i0,r1,i1,r2,i2,r3,i3] = [0,2,4,6].
		// odd  -> [i0,i1,i2,i3] map is [1,3,5,7].
		__m256d re256 = _mm512_castpd512_pd256(v_re);
		__m256d im256 = _mm512_castpd512_pd256(v_im);

		_mm256_storeu_pd(re + j, re256);
		_mm256_storeu_pd(im + j, im256);
	}
#elif defined(__AVX2__)
	// 4 complex per iter using 256-bit path:
	// a = [r0,i0,r1,i1], b = [r2,i2,r3,i3]
	// re_temp = shuffle_pd(a,b,0x0) -> [r0,r2, r1,r3]
	// im_temp = shuffle_pd(a,b,0xF) -> [i0,i2, i1,i3]
	// Then swap lanes 1<->2 with permute4x64 imm=0xD8 to get [r0,r1,r2,r3] and [i0,i1,i2,i3]
	for (; j + 4 <= n_complex; j += 4) {
		__m256d a = _mm256_loadu_pd(s + (size_t)j * 2);
		__m256d b = _mm256_loadu_pd(s + (size_t)j * 2 + 4);
		__m256d re_tmp = _mm256_shuffle_pd(a, b, 0x0);
		__m256d im_tmp = _mm256_shuffle_pd(a, b, 0xF);
		__m256d re_vec = _mm256_permute4x64_pd(re_tmp, 0xD8); // [0,2,1,3] -> [0,1,2,3]
		__m256d im_vec = _mm256_permute4x64_pd(im_tmp, 0xD8);
		_mm256_storeu_pd(re + j, re_vec);
		_mm256_storeu_pd(im + j, im_vec);
	}
#elif defined(__AVX__)
	// 2 complex per iter using 128-bit SSE ops (valid in AVX TU):
	// a = [r0,i0,r1,i1]
	// re = shuffle(a, a, 0b1000) -> [r0,r1]
	// im = shuffle(a, a, 0b1111) with mask picking odds -> [i0,i1]
	for (; j + 2 <= n_complex; j += 2) {
		__m128d lo = _mm_loadu_pd(s + (size_t)j * 2);         // [r0,i0]
		__m128d hi = _mm_loadu_pd(s + (size_t)j * 2 + 2);     // [r1,i1]
		__m128d a = _mm_shuffle_pd(lo, hi, 0b00);            // [r0,r1]
		__m128d b = _mm_shuffle_pd(lo, hi, 0b11);            // [i0,i1]
		_mm_storeu_pd(re + j, a);
		_mm_storeu_pd(im + j, b);
	}
#endif

	// Tail
	for (; j < n_complex; ++j) {
		re[j] = s[2 * (size_t)j + 0];
		im[j] = s[2 * (size_t)j + 1];
	}
}
void hcInitSingle(HConvSingle * filter, double* h, int hlen, int flen, int steps)
{
	int i, j, size, num, pos;
	double gain;

	filter->step = 0;
	filter->maxstep = steps;
	filter->mixpos = 0;
	filter->framelength = flen;

	size = sizeof(double) * 2 * flen;
	filter->dft_time = (double*)fftw_malloc(size);

	size = sizeof(fftw_complex) * (flen + 1);
	filter->dft_freq = (fftw_complex*)fftw_malloc(size);

	size = sizeof(double) * (flen + 1);
	filter->in_freq_real = (double*)fftw_malloc(size);
	filter->in_freq_imag = (double*)fftw_malloc(size);

	filter->num_filterbuf = (hlen + flen - 1) / flen;

	size = sizeof(int) * (steps + 1);
	filter->steptask = (int*)malloc(size);
	num = filter->num_filterbuf / steps;
	for (i = 0; i <= steps; i++)
		filter->steptask[i] = i * num;
	pos = (filter->steptask[1] == 0) ? 1 : 2;
	num = filter->num_filterbuf % steps;
	for (j = pos; j < pos + num; j++) {
		for (i = j; i <= steps; i++)
			filter->steptask[i]++;
	}

	size = sizeof(double*) * filter->num_filterbuf;
	filter->filterbuf_freq_real = (double**)fftw_malloc(size);
	filter->filterbuf_freq_imag = (double**)fftw_malloc(size);
	for (i = 0; i < filter->num_filterbuf; i++) {
		size = sizeof(double) * (flen + 1);
		filter->filterbuf_freq_real[i] = (double*)fftw_malloc(size);
		filter->filterbuf_freq_imag[i] = (double*)fftw_malloc(size);
	}

	filter->num_mixbuf = filter->num_filterbuf + 1;

	size = sizeof(double*) * filter->num_mixbuf;
	filter->mixbuf_freq_real = (double**)fftw_malloc(size);
	filter->mixbuf_freq_imag = (double**)fftw_malloc(size);
	for (i = 0; i < filter->num_mixbuf; i++) {
		size = sizeof(double) * (flen + 1);
		filter->mixbuf_freq_real[i] = (double*)fftw_malloc(size);
		filter->mixbuf_freq_imag[i] = (double*)fftw_malloc(size);
		memset(filter->mixbuf_freq_real[i], 0, size);
		memset(filter->mixbuf_freq_imag[i], 0, size);
	}

	size = sizeof(double) * flen;
	filter->history_time = (double*)fftw_malloc(size);
	memset(filter->history_time, 0, size);

	// Use FFTW_MEASURE for production for potentially higher performance, at the cost of a longer setup time.
	// FFTW_ESTIMATE is faster for setup.
	unsigned fftw_flags = FFTW_ESTIMATE | FFTW_PRESERVE_INPUT;
	filter->fft = fftw_plan_dft_r2c_1d(2 * flen, filter->dft_time, filter->dft_freq, fftw_flags);
	filter->ifft = fftw_plan_dft_c2r_1d(2 * flen, filter->dft_freq, filter->dft_time, fftw_flags);

	gain = 0.5 / flen;

	memset(filter->dft_time, 0, sizeof(double) * 2 * flen);

	// Full-length segments
	for (i = 0; i < filter->num_filterbuf - 1; i++) {
		// dft_time[0:flen] = gain * h[i * flen + 0 : + flen]
		mul_store_gain_double(filter->dft_time, h + (size_t)i * flen, flen, gain);

		fftw_execute(filter->fft);

		// Split complex to separate real/imag buffers
		copy_split_complex_vec((const fftw_complex*)filter->dft_freq,
			filter->filterbuf_freq_real[i],
			filter->filterbuf_freq_imag[i],
			flen + 1);
	}

	// Tail (possibly partial) segment
	int last_segment_len = hlen - i * flen;
	if (last_segment_len > 0) {
		mul_store_gain_double(filter->dft_time, h + (size_t)i * flen, last_segment_len, gain);
		// zero the remainder up to 2*flen
		memset(&filter->dft_time[last_segment_len], 0,
			sizeof(double) * (2 * (size_t)flen - (size_t)last_segment_len));
	}
	else {
		// No tail data: ensure the time buffer is zeroed
		memset(filter->dft_time, 0, sizeof(double) * 2 * flen);
	}

	fftw_execute(filter->fft);
	copy_split_complex_vec((const fftw_complex*)filter->dft_freq,
		filter->filterbuf_freq_real[i],
		filter->filterbuf_freq_imag[i],
		flen + 1);
}

void hcCloseSingle(HConvSingle* filter)
{
	fftw_destroy_plan(filter->ifft);
	fftw_destroy_plan(filter->fft);
	fftw_free(filter->history_time);
	for (int i = 0; i < filter->num_mixbuf; i++) {
		fftw_free(filter->mixbuf_freq_real[i]);
		fftw_free(filter->mixbuf_freq_imag[i]);
	}
	fftw_free(filter->mixbuf_freq_real);
	fftw_free(filter->mixbuf_freq_imag);
	for (int i = 0; i < filter->num_filterbuf; i++) {
		fftw_free(filter->filterbuf_freq_real[i]);
		fftw_free(filter->filterbuf_freq_imag[i]);
	}
	fftw_free(filter->filterbuf_freq_real);
	fftw_free(filter->filterbuf_freq_imag);
	fftw_free(filter->in_freq_real);
	fftw_free(filter->in_freq_imag);
	fftw_free(filter->dft_freq);
	fftw_free(filter->dft_time);
	free(filter->steptask);
	memset(filter, 0, sizeof(HConvSingle));
}


void hcBenchmarkDual(int sflen, int lflen)
{
	HConvDual filter;
	double* x, * h, * y;
	int xlen, hlen, ylen, size, n, pos;
	double t_start, t_diff, counter = 0.0, signal_time, cpu_load, lin, mul;

	xlen = 2048 * 2048;
	size = sizeof(double) * xlen;
	x = (double*)fftw_malloc(size);
	lin = pow(10.0, -100.0 / 20.0);
	mul = pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (n = 1; n < xlen; n++)
		x[n] = (double)(-mul * x[n - 1]);

	hlen = 96000;
	size = sizeof(double) * hlen;
	h = (double*)fftw_malloc(size);
	lin = pow(10.0, -60.0 / 20.0);
	mul = pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (n = 1; n < hlen; n++)
		h[n] = (double)(mul * h[n - 1]);

	ylen = sflen;
	size = sizeof(double) * ylen;
	y = (double*)fftw_malloc(size);

	hcInitDual(&filter, h, hlen, sflen, lflen);

	t_diff = 0.0;
	t_start = hcTime();
	pos = 0;
	while (t_diff < 10.0) {
		hcProcessDual(&filter, &(x[pos]), y);
		pos += sflen;
		if (pos >= xlen) pos = 0;
		counter += 1.0;
		t_diff = hcTime() - t_start;
	}
	signal_time = counter * sflen / 48000.0;
	cpu_load = 100.0 * t_diff / signal_time;
	printf("Estimated CPU load: %5.2f %%\n", cpu_load);

	hcCloseDual(&filter);
	fftw_free(x);
	fftw_free(h);
	fftw_free(y);
}


void hcProcessDual(HConvDual* filter, double* in, double* out)
{
	// This function calls the optimized single-filter functions
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetSingle(filter->f_short, out);

	const int lpos = filter->step * filter->flen_short;
	for (int i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_long[lpos + i];

	if (filter->step == 0)
		hcPutSingle(filter->f_long, filter->in_long);
	hcProcessSingle(filter->f_long);
	if (filter->step == filter->maxstep - 1)
		hcGetSingle(filter->f_long, filter->out_long);

	memcpy(&(filter->in_long[lpos]), in, sizeof(double) * filter->flen_short);
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcProcessAddDual(HConvDual* filter, double* in, double* out)
{
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetAddSingle(filter->f_short, out);

	const int lpos = filter->step * filter->flen_short;
	for (int i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_long[lpos + i];

	if (filter->step == 0)
		hcPutSingle(filter->f_long, filter->in_long);
	hcProcessSingle(filter->f_long);
	if (filter->step == filter->maxstep - 1)
		hcGetSingle(filter->f_long, filter->out_long);

	memcpy(&(filter->in_long[lpos]), in, sizeof(double) * filter->flen_short);
	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcInitDual(HConvDual* filter, double* h, int hlen, int sflen, int lflen)
{
	int size;
	double* h2 = NULL;
	int h2len = 2 * lflen + 1;

	if (hlen < h2len) {
		size = sizeof(double) * h2len;
		h2 = (double*)fftw_malloc(size);
		memset(h2, 0, size);
		memcpy(h2, h, sizeof(double) * hlen);
		h = h2;
		hlen = h2len;
	}

	filter->step = 0;
	filter->maxstep = lflen / sflen;
	filter->flen_long = lflen;
	filter->flen_short = sflen;

	size = sizeof(double) * lflen;
	filter->in_long = (double*)fftw_malloc(size);
	memset(filter->in_long, 0, size);
	filter->out_long = (double*)fftw_malloc(size);
	memset(filter->out_long, 0, size);

	filter->f_short = (HConvSingle*)malloc(sizeof(HConvSingle));
	hcInitSingle(filter->f_short, h, 2 * lflen, sflen, 1);

	filter->f_long = (HConvSingle*)malloc(sizeof(HConvSingle));
	hcInitSingle(filter->f_long, &(h[2 * lflen]), hlen - 2 * lflen, lflen, lflen / sflen);

	if (h2 != NULL) {
		fftw_free(h2);
	}
}


void hcCloseDual(HConvDual* filter)
{
	hcCloseSingle(filter->f_short);
	free(filter->f_short);
	hcCloseSingle(filter->f_long);
	free(filter->f_long);
	fftw_free(filter->out_long);
	fftw_free(filter->in_long);
	memset(filter, 0, sizeof(HConvDual));
}


////////////////////////////////////////////////////////////////


void hcBenchmarkTripple(int sflen, int mflen, int lflen)
{
	HConvTripple filter;
	double* x, * h, * y;
	int xlen, hlen, ylen, size, n, pos;
	double t_start, t_diff, counter = 0.0, signal_time, cpu_load, lin, mul;

	xlen = 2048 * 2048;
	size = sizeof(double) * xlen;
	x = (double*)fftw_malloc(size);
	lin = pow(10.0, -100.0 / 20.0);
	mul = pow(lin, 1.0 / (double)xlen);
	x[0] = 1.0;
	for (n = 1; n < xlen; n++)
		x[n] = (double)(-mul * x[n - 1]);

	hlen = 96000;
	size = sizeof(double) * hlen;
	h = (double*)fftw_malloc(size);
	lin = pow(10.0, -60.0 / 20.0);
	mul = pow(lin, 1.0 / (double)hlen);
	h[0] = 1.0;
	for (n = 1; n < hlen; n++)
		h[n] = (double)(mul * h[n - 1]);

	ylen = sflen;
	size = sizeof(double) * ylen;
	y = (double*)fftw_malloc(size);

	hcInitTripple(&filter, h, hlen, sflen, mflen, lflen);

	t_diff = 0.0;
	t_start = hcTime();
	size = mflen / sflen;
	pos = 0;
	while (t_diff < 10.0) {
		for (n = 0; n < size; n++) {
			hcProcessTripple(&filter, &(x[pos]), y);
			pos += sflen;
			if (pos >= xlen) pos = 0;
		}
		counter += 1.0;
		t_diff = hcTime() - t_start;
	}
	signal_time = counter * (double)mflen / 48000.0;
	cpu_load = 100.0 * t_diff / signal_time;
	printf("Estimated CPU load: %5.2f %%\n", cpu_load);

	hcCloseTripple(&filter);
	fftw_free(x);
	fftw_free(h);
	fftw_free(y);
}


void hcProcessTripple(HConvTripple* filter, double* in, double* out)
{
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetSingle(filter->f_short, out);

	const int lpos = filter->step * filter->flen_short;
	for (int i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_medium[lpos + i];

	memcpy(&(filter->in_medium[lpos]), in, sizeof(double) * filter->flen_short);

	if (filter->step == filter->maxstep - 1)
		hcProcessDual(filter->f_medium, filter->in_medium, filter->out_medium);

	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcProcessAddTripple(HConvTripple* filter, double* in, double* out)
{
	hcPutSingle(filter->f_short, in);
	hcProcessSingle(filter->f_short);
	hcGetAddSingle(filter->f_short, out);

	const int lpos = filter->step * filter->flen_short;
	for (int i = 0; i < filter->flen_short; i++)
		out[i] += filter->out_medium[lpos + i];

	memcpy(&(filter->in_medium[lpos]), in, sizeof(double) * filter->flen_short);

	if (filter->step == filter->maxstep - 1)
		hcProcessDual(filter->f_medium, filter->in_medium, filter->out_medium);

	filter->step = (filter->step + 1) % filter->maxstep;
}


void hcInitTripple(HConvTripple* filter, double* h, int hlen, int sflen, int mflen, int lflen)
{
	int size;
	double* h2 = NULL;
	int h2len = mflen + 2 * lflen + 1;

	if (hlen < h2len) {
		size = sizeof(double) * h2len;
		h2 = (double*)fftw_malloc(size);
		memset(h2, 0, size);
		memcpy(h2, h, sizeof(double) * hlen);
		h = h2;
		hlen = h2len;
	}

	filter->step = 0;
	filter->maxstep = mflen / sflen;
	filter->flen_medium = mflen;
	filter->flen_short = sflen;

	size = sizeof(double) * mflen;
	filter->in_medium = (double*)fftw_malloc(size);
	memset(filter->in_medium, 0, size);
	filter->out_medium = (double*)fftw_malloc(size);
	memset(filter->out_medium, 0, size);

	filter->f_short = (HConvSingle*)malloc(sizeof(HConvSingle));
	hcInitSingle(filter->f_short, h, mflen, sflen, 1);

	filter->f_medium = (HConvDual*)malloc(sizeof(HConvDual));
	hcInitDual(filter->f_medium, &(h[mflen]), hlen - mflen, mflen, lflen);

	if (h2 != NULL) {
		fftw_free(h2);
	}
}


void hcCloseTripple(HConvTripple* filter)
{
	hcCloseSingle(filter->f_short);
	free(filter->f_short);
	hcCloseDual(filter->f_medium);
	free(filter->f_medium);
	fftw_free(filter->out_medium);
	fftw_free(filter->in_medium);
	memset(filter, 0, sizeof(HConvTripple));
}
