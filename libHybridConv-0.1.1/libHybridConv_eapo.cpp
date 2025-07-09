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
#include <xmmintrin.h>
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
	DWORD t = GetTickCount();
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
	const size_t flen = filter->framelength;
	const size_t dft_len = 2 * flen;
	const size_t freq_len = flen + 1;

	// --- Phase 1: Input Preparation (Copy input signal and zero-pad) ---
	// Process the entire dft_time buffer, copying the first half and zeroing the second.
	size_t n = 0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
	{
		const size_t simd_width = 8;
		const __m512d zero_vec = _mm512_setzero_pd();
		for (; n + simd_width <= flen; n += simd_width) {
			_mm512_storeu_pd(filter->dft_time + n, _mm512_loadu_pd(x + n));
		}
		for (; n + simd_width <= dft_len; n += simd_width) {
			_mm512_storeu_pd(filter->dft_time + n, zero_vec);
		}
	}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
	{
		const size_t simd_width = 4;
		const __m256d zero_vec = _mm256_setzero_pd();
		for (; n + simd_width <= flen; n += simd_width) {
			_mm256_storeu_pd(filter->dft_time + n, _mm256_loadu_pd(x + n));
		}
		for (; n + simd_width <= dft_len; n += simd_width) {
			_mm256_storeu_pd(filter->dft_time + n, zero_vec);
		}
	}
#endif

#if !defined(_M_ARM64)
	{
		const size_t simd_width = 2;
		const __m128d zero_vec = _mm_setzero_pd();
		for (; n + simd_width <= flen; n += simd_width) {
			_mm_storeu_pd(filter->dft_time + n, _mm_loadu_pd(x + n));
		}
		for (; n + simd_width <= dft_len; n += simd_width) {
			_mm_storeu_pd(filter->dft_time + n, zero_vec);
		}
	}
#endif

	// For any remaining elements, use the standard library functions.
	if (n < flen) {
		memcpy(filter->dft_time + n, x + n, (flen - n) * sizeof(double));
	}
	// The start of the zero-padding is max(n, flen).
	n = (n > flen) ? n : flen;
	if (n < dft_len) {
		memset(filter->dft_time + n, 0, (dft_len - n) * sizeof(double));
	}


	// --- Phase 2: Perform the FFT ---
	fftw_execute(filter->fft);


	// --- Phase 3: De-interleave FFTW's complex output into planar arrays ---
	// The pattern is: load two vectors, unpack, permute, store.
	size_t j = 0;
	fftw_complex* dft_freq = filter->dft_freq;

#if defined(__AVX512F__) && !defined(_M_ARM64)
	{
		// Process 8 complex numbers (16 doubles) per loop.
		const size_t complex_width = 8;
		for (; j + complex_width <= freq_len; j += complex_width) {
			// Load 8 complex numbers from two adjacent memory locations.
			__m512d cplx0 = _mm512_load_pd((double*)(dft_freq + j));     // Contains c0, c1, c2, c3
			__m512d cplx1 = _mm512_load_pd((double*)(dft_freq + j + 4)); // Contains c4, c5, c6, c7

			// Unpack to group alternating real and imaginary parts.
			__m512d riri0 = _mm512_unpacklo_pd(cplx0, cplx1); // [r0,r4, r1,r5, i0,i4, i1,i5]
			__m512d riri1 = _mm512_unpackhi_pd(cplx0, cplx1); // [r2,r6, r3,r7, i2,i6, i3,i7]

			// Create final vectors by selecting the correct 64-bit lanes from the unpacked results.
			// This is a 2-input permutation. The index selects from the 16 available lanes (8 from a, 8 from b).
			const __m512i P_REAL_IDX = _mm512_set_epi64(11, 9, 3, 1, 10, 8, 2, 0); // Indices for {r7,r6,r5,r4,r3,r2,r1,r0}
			const __m512i P_IMAG_IDX = _mm512_set_epi64(15, 13, 7, 5, 14, 12, 6, 4); // Indices for {i7,i6,i5,i4,i3,i2,i1,i0}
			__m512d real_vec = _mm512_permutex2var_pd(riri0, P_REAL_IDX, riri1);
			__m512d imag_vec = _mm512_permutex2var_pd(riri0, P_IMAG_IDX, riri1);

			_mm512_storeu_pd(filter->in_freq_real + j, real_vec);
			_mm512_storeu_pd(filter->in_freq_imag + j, imag_vec);
		}
	}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
	{
		// Process 4 complex numbers (8 doubles) per loop.
		const size_t complex_width = 4;
		for (; j + complex_width <= freq_len; j += complex_width) {
			// Load 4 complex numbers from two adjacent memory locations.
			__m256d cplx0 = _mm256_load_pd((double*)(dft_freq + j));     // Contains c0, c1
			__m256d cplx1 = _mm256_load_pd((double*)(dft_freq + j + 2)); // Contains c2, c3

			// Unpack to group alternating real and imaginary parts.
			__m256d riri0 = _mm256_unpacklo_pd(cplx0, cplx1); // [r0, r2, i0, i2]
			__m256d riri1 = _mm256_unpackhi_pd(cplx0, cplx1); // [r1, r3, i1, i3]

			// Permute the 128-bit lanes to gather all real and imaginary parts.
			// The mask 0x20 = 0b00100000 -> lane0 from argB, lane0 from argA
			__m256d real_vec = _mm256_permute2f128_pd(riri0, riri1, 0x20); // [r0, r2, r1, r3]
			// The mask 0x31 = 0b00110001 -> lane1 from argB, lane1 from argA
			__m256d imag_vec = _mm256_permute2f128_pd(riri0, riri1, 0x31); // [i0, i2, i1, i3]

			// A final shuffle on each vector fixes the order.
			// Mask 0b0101 swaps the middle two elements of the 256-bit vector.
			real_vec = _mm256_permute_pd(real_vec, 0x05); // [r0, r1, r2, r3]
			imag_vec = _mm256_permute_pd(imag_vec, 0x05); // [i0, i1, i2, i3]

			_mm256_storeu_pd(filter->in_freq_real + j, real_vec);
			_mm256_storeu_pd(filter->in_freq_imag + j, imag_vec);
		}
	}
#endif

#if !defined(_M_ARM64)
	{
		// Process 2 complex numbers (4 doubles) per loop.
		const size_t complex_width = 2;
		for (; j + complex_width <= freq_len; j += complex_width) {
			// Load 2 complex numbers.
			__m128d cplx0 = _mm_load_pd((double*)(dft_freq + j));     // [r0, i0]
			__m128d cplx1 = _mm_load_pd((double*)(dft_freq + j + 1)); // [r1, i1]

			// A single shuffle is sufficient for the 128-bit case.
			// Mask 0b00 grabs element 0 from each source.
			__m128d real_vec = _mm_shuffle_pd(cplx0, cplx1, 0x00); // [r0, r1]
			// Mask 0b11 grabs element 1 from each source.
			__m128d imag_vec = _mm_shuffle_pd(cplx0, cplx1, 0x03); // [i0, i1]

			_mm_storeu_pd(filter->in_freq_real + j, real_vec);
			_mm_storeu_pd(filter->in_freq_imag + j, imag_vec);
		}
	}
#endif

	// Scalar Fallback for any remaining complex numbers (at most 1).
	for (; j < freq_len; j++) {
		filter->in_freq_real[j] = dft_freq[j][0];
		filter->in_freq_imag[j] = dft_freq[j][1];
	}
}

void hcProcessSingle(HConvSingle* filter)
{
	const int flen = filter->framelength;
	// The number of complex elements is framelength / 2 + 1.
	// The number of double elements is 2 * (framelength / 2 + 1) = framelength + 2.
	const int num_elements = flen + 1;
	const double* const x_real = filter->in_freq_real;
	const double* const x_imag = filter->in_freq_imag;

	const int start = filter->steptask[filter->step];
	const int stop = filter->steptask[filter->step + 1];

	for (int s = start; s < stop; s++) {
		const int mix_idx = (s + filter->mixpos) % filter->num_mixbuf;
		double* const y_real = filter->mixbuf_freq_real[mix_idx];
		double* const y_imag = filter->mixbuf_freq_imag[mix_idx];
		const double* const h_real = filter->filterbuf_freq_real[s];
		const double* const h_imag = filter->filterbuf_freq_imag[s];

#if !defined(_M_ARM64)
		// Prefetch the filter data for the *next* segment to hide memory latency.
		if (s + 1 < stop) {
			_mm_prefetch((char const*)(filter->filterbuf_freq_real[s + 1]), _MM_HINT_T0);
			_mm_prefetch((char const*)(filter->filterbuf_freq_imag[s + 1]), _MM_HINT_T0);
		}
#endif
		size_t n = 0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
		// AVX-512 Path: Process 8 doubles (4 complex numbers) at a time.
		{
			const size_t simd_width = 8;
			if (num_elements - n >= simd_width) {
				for (; n + simd_width <= num_elements; n += simd_width) {
					const __m512d xr = _mm512_loadu_pd(x_real + n);
					const __m512d xi = _mm512_loadu_pd(x_imag + n);
					const __m512d hr = _mm512_loadu_pd(h_real + n);
					const __m512d hi = _mm512_loadu_pd(h_imag + n);
					__m512d yr = _mm512_loadu_pd(y_real + n);
					__m512d yi = _mm512_loadu_pd(y_imag + n);

					// Real part: yr += xr*hr - xi*hi
					yr = _mm512_fmadd_pd(xr, hr, yr);
					yr = _mm512_fnmadd_pd(xi, hi, yr);

					// Imaginary part: yi += xr*hi + xi*hr
					yi = _mm512_fmadd_pd(xr, hi, yi);
					yi = _mm512_fmadd_pd(xi, hr, yi);

					_mm512_storeu_pd(y_real + n, yr);
					_mm512_storeu_pd(y_imag + n, yi);
				}
			}
		}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
		// AVX2 Path: Process 4 doubles (2 complex numbers) from the remainder.
		{
			const size_t simd_width = 4;
			if (num_elements - n >= simd_width) {
				// This loop will run at most once.
				for (; n + simd_width <= num_elements; n += simd_width) {
					const __m256d xr = _mm256_loadu_pd(x_real + n);
					const __m256d xi = _mm256_loadu_pd(x_imag + n);
					const __m256d hr = _mm256_loadu_pd(h_real + n);
					const __m256d hi = _mm256_loadu_pd(h_imag + n);
					__m256d yr = _mm256_loadu_pd(y_real + n);
					__m256d yi = _mm256_loadu_pd(y_imag + n);

					yr = _mm256_fmadd_pd(xr, hr, yr);
					yr = _mm256_fnmadd_pd(xi, hi, yr);
					yi = _mm256_fmadd_pd(xr, hi, yi);
					yi = _mm256_fmadd_pd(xi, hr, yi);

					_mm256_storeu_pd(y_real + n, yr);
					_mm256_storeu_pd(y_imag + n, yi);
				}
			}
		}
#endif

#if !defined(_M_ARM64)
		// SSE/AVX 128-bit Path: Process 2 doubles (1 complex number) from the remainder.
		{
			const size_t simd_width = 2;
			if (num_elements - n >= simd_width) {
				// This loop will run at most once.
				for (; n + simd_width <= num_elements; n += simd_width) {
					const __m128d xr = _mm_loadu_pd(x_real + n);
					const __m128d xi = _mm_loadu_pd(x_imag + n);
					const __m128d hr = _mm_loadu_pd(h_real + n);
					const __m128d hi = _mm_loadu_pd(h_imag + n);
					__m128d yr = _mm_loadu_pd(y_real + n);
					__m128d yi = _mm_loadu_pd(y_imag + n);
#if defined(__AVX2__)
					yr = _mm_fmadd_pd(xr, hr, yr);
					yr = _mm_fnmadd_pd(xi, hi, yr);
					yi = _mm_fmadd_pd(xr, hi, yi);
					yi = _mm_fmadd_pd(xi, hr, yi);
#else // Fallback for pure SSE2 (no FMA)
					__m128d real_part = _mm_sub_pd(_mm_mul_pd(xr, hr), _mm_mul_pd(xi, hi));
					__m128d imag_part = _mm_add_pd(_mm_mul_pd(xr, hi), _mm_mul_pd(xi, hr));
					yr = _mm_add_pd(yr, real_part);
					yi = _mm_add_pd(yi, imag_part);
#endif
					_mm_storeu_pd(y_real + n, yr);
					_mm_storeu_pd(y_imag + n, yi);
				}
			}
		}
#endif
		// Scalar Fallback: Process the final remaining element, if any.
		for (; n < num_elements; ++n) {
			y_real[n] += x_real[n] * h_real[n] - x_imag[n] * h_imag[n];
			y_imag[n] += x_real[n] * h_imag[n] + x_imag[n] * h_real[n];
		}
	}
	filter->step = (filter->step + 1) % filter->maxstep;
}

void hcGetSingleImpl(HConvSingle* filter, double* y, bool additive)
{
	const int flen = filter->framelength;
	const int freq_len = flen + 1;
	const int mpos = filter->mixpos;
	double* const out = filter->dft_time;
	double* const hist = filter->history_time;
	double* const mix_real = filter->mixbuf_freq_real[mpos];
	double* const mix_imag = filter->mixbuf_freq_imag[mpos];

#if defined(__AVX512F__) && !defined(_M_ARM64)
	// Vectorized loop to re-interleave planar real/imag data and zero the source buffers.
	int j = 0;
	for (; j <= freq_len - 4; j += 4) { // Process 4 complex numbers
		__m256d real_part = _mm256_loadu_pd(mix_real + j); // [r0, r1, r2, r3]
		__m256d imag_part = _mm256_loadu_pd(mix_imag + j); // [i0, i1, i2, i3]

		// Interleave to [r0, i0, r1, i1]
		__m256d cplx_lo = _mm256_unpacklo_pd(real_part, imag_part);
		// Interleave to [r2, i2, r3, i3]
		__m256d cplx_hi = _mm256_unpackhi_pd(real_part, imag_part);

		// Combine into a single 512-bit vector for storing
		__m512d cplx = _mm512_setzero_pd();
		cplx = _mm512_insertf64x4(cplx, cplx_lo, 0);
		cplx = _mm512_insertf64x4(cplx, cplx_hi, 1);

		_mm512_store_pd((double*)(filter->dft_freq + j), cplx);

		// Zero out the source mixing buffers
		_mm256_storeu_pd(mix_real + j, _mm256_setzero_pd());
		_mm256_storeu_pd(mix_imag + j, _mm256_setzero_pd());
	}
	// Scalar remainder
	for (; j < freq_len; j++) {
		filter->dft_freq[j][0] = mix_real[j];
		filter->dft_freq[j][1] = mix_imag[j];
		mix_real[j] = 0.0;
		mix_imag[j] = 0.0;
	}
#else // Scalar Fallback
	for (int j = 0; j < freq_len; j++) {
		filter->dft_freq[j][0] = mix_real[j];
		filter->dft_freq[j][1] = mix_imag[j];
		mix_real[j] = 0.0;
		mix_imag[j] = 0.0;
	}
#endif

	fftw_execute(filter->ifft);

#if defined(__AVX2__) && !defined(_M_ARM64)
	// Vectorized main output calculation (overlap-add)
	int n = 0;
	const int vec_size = 4;
	if (additive) {
		for (; n <= flen - vec_size; n += vec_size) {
			const __m256d y_vec = _mm256_loadu_pd(y + n);
			const __m256d out_vec = _mm256_loadu_pd(out + n);
			const __m256d hist_vec = _mm256_loadu_pd(hist + n);
			const __m256d sum = _mm256_add_pd(out_vec, hist_vec);
			_mm256_storeu_pd(y + n, _mm256_add_pd(y_vec, sum));
		}
	}
	else {
		for (; n <= flen - vec_size; n += vec_size) {
			const __m256d out_vec = _mm256_loadu_pd(out + n);
			const __m256d hist_vec = _mm256_loadu_pd(hist + n);
			_mm256_storeu_pd(y + n, _mm256_add_pd(out_vec, hist_vec));
		}
	}
	// Vectorized history buffer update
	for (n = 0; n <= flen - vec_size; n += vec_size) {
		_mm256_storeu_pd(hist + n, _mm256_loadu_pd(out + flen + n));
	}
	// Use a scalar loop for all remaining operations to keep it simple.
	if (additive) {
		for (n = (flen / vec_size) * vec_size; n < flen; n++) y[n] += out[n] + hist[n];
	}
	else {
		for (n = (flen / vec_size) * vec_size; n < flen; n++) y[n] = out[n] + hist[n];
	}
	for (n = (flen / vec_size) * vec_size; n < flen; ++n) hist[n] = out[flen + n];

#else // Scalar Fallback
	if (additive) {
		for (int n = 0; n < flen; n++) y[n] += out[n] + hist[n];
	}
	else {
		for (int n = 0; n < flen; n++) y[n] = out[n] + hist[n];
	}
	memcpy(hist, &(out[flen]), sizeof(double) * flen);
#endif

	filter->mixpos = (filter->mixpos + 1) % filter->num_mixbuf;
}

void hcGetSingle(HConvSingle* filter, double* y)
{
	hcGetSingleImpl(filter, y, false);
}

void hcGetAddSingle(HConvSingle* filter, double* y)
{
	hcGetSingleImpl(filter, y, true);
}
void hcInitSingle(HConvSingle* filter, double* h, int hlen, int flen, int steps)
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
	for (i = 0; i < filter->num_filterbuf - 1; i++) {
		for (j = 0; j < flen; j++)
			filter->dft_time[j] = gain * h[i * flen + j];
		fftw_execute(filter->fft);
		for (j = 0; j < flen + 1; j++) {
			filter->filterbuf_freq_real[i][j] = filter->dft_freq[j][0];
			filter->filterbuf_freq_imag[i][j] = filter->dft_freq[j][1];
		}
	}

	int last_segment_len = hlen - i * flen;
	for (j = 0; j < last_segment_len; j++)
		filter->dft_time[j] = gain * h[i * flen + j];
	memset(&(filter->dft_time[last_segment_len]), 0, sizeof(double) * (2 * flen - last_segment_len));

	fftw_execute(filter->fft);
	for (j = 0; j < flen + 1; j++) {
		filter->filterbuf_freq_real[i][j] = filter->dft_freq[j][0];
		filter->filterbuf_freq_imag[i][j] = filter->dft_freq[j][1];
	}
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
