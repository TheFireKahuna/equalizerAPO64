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
#include <algorithm>
#include <cmath> // For std::cos

#ifndef _M_ARM64
#include <immintrin.h>
#endif

#include "FilterEngine.h"
#include "helpers/MemoryHelper.h"
#include "FilterConfiguration.h"

using namespace std;
// Deinterleave interleaved stereo (LRLR...) -> L[0..], R[0..]
static inline void deinterleave_stereo_scalar(const double* src, double* __restrict L, double* __restrict R, unsigned nframes) {
	for (unsigned i = 0; i < nframes; ++i) {
		L[i] = src[2u * i + 0];
		R[i] = src[2u * i + 1];
	}
}

// Interleave stereo L/R -> (LRLR...)
static inline void interleave_stereo_scalar(const double* __restrict L, const double* __restrict R, double* dst, unsigned nframes) {
	for (unsigned i = 0; i < nframes; ++i) {
		dst[2u * i + 0] = L[i];
		dst[2u * i + 1] = R[i];
	}
}

#if defined(__AVX512F__)
// AVX-512F: process 4 frames (8 doubles) per ZMM using compress/expand.
// Masks for even (L) and odd (R) lanes over 8 elements: even=0b10101010 (0xAA), odd=0b01010101 (0x55)
static inline void deinterleave_stereo_avx512(const double* src, double* __restrict L, double* __restrict R, unsigned nframes) {
	const unsigned step = 4; // frames per 512b chunk (8 doubles)
	const unsigned n4 = (nframes / step) * step;
	const __mmask8 mask_even = 0x55; // lanes 0,2,4,6 -> L
	const __mmask8 mask_odd = 0xAA; // lanes 1,3,5,7 -> R

	unsigned i = 0;
	for (; i < n4; i += step) {
		__m512d v = _mm512_loadu_pd(src + (size_t)2 * i); // load 8 doubles: L0 R0 L1 R1 L2 R2 L3 R3
		_mm512_mask_compressstoreu_pd(L + i, mask_even, v); // write L0 L1 L2 L3
		_mm512_mask_compressstoreu_pd(R + i, mask_odd, v); // write R0 R1 R2 R3
	}
	if (i < nframes) {
		deinterleave_stereo_scalar(src + 2u * i, L + i, R + i, nframes - i);
	}
}

static inline void interleave_stereo_avx512(const double* __restrict L, const double* __restrict R, double* dst, unsigned nframes) {
	const unsigned step = 4; // frames per 512b chunk (8 doubles)
	const unsigned n4 = (nframes / step) * step;
	const __mmask8 mask_even = 0x55; // lanes 0,2,4,6 -> L
	const __mmask8 mask_odd = 0xAA; // lanes 1,3,5,7 -> R

	unsigned i = 0;
	for (; i < n4; i += step) {
		__m512d v = _mm512_setzero_pd();
		// Expand-load L into even lanes
		v = _mm512_mask_expandloadu_pd(v, mask_even, L + i); // places L0 L1 L2 L3 into lanes 0,2,4,6
		// Expand-load R into odd lanes
		v = _mm512_mask_expandloadu_pd(v, mask_odd, R + i); // places R0 R1 R2 R3 into lanes 1,3,5,7
		_mm512_storeu_pd(dst + (size_t)2 * i, v); // store LRLR...
	}
	if (i < nframes) {
		interleave_stereo_scalar(L + i, R + i, dst + 2u * i, nframes - i);
	}
}
#endif // __AVX512F__

#if defined(__AVX2__) || defined(__AVX__)
// AVX/AVX2 256-bit: process 4 frames per iteration (8 doubles total via two 256b loads)

// Deinterleave:
// a = [L0 R0 L1 R1], b = [L2 R2 L3 R3]
// pa = permute2f128(a,b,0x20) -> [L0 R0 L2 R2]
// pb = permute2f128(a,b,0x31) -> [L1 R1 L3 R3]
// left  = shuffle_pd(pa,pb,0x0) -> [L0 L1 L2 L3]
// right = shuffle_pd(pa,pb,0xF) -> [R0 R1 R2 R3]
static inline void deinterleave_stereo_avx(const double* src, double* __restrict L, double* __restrict R, unsigned nframes) {
	const unsigned step = 4; // frames per 256b sequence (two loads)
	const unsigned n4 = (nframes / step) * step;

	unsigned i = 0;
	for (; i < n4; i += step) {
		__m256d a = _mm256_loadu_pd(src + (size_t)2 * i + 0); // L0 R0 L1 R1
		__m256d b = _mm256_loadu_pd(src + (size_t)2 * i + 4); // L2 R2 L3 R3

		__m256d pa = _mm256_permute2f128_pd(a, b, 0x20);    // L0 R0 L2 R2
		__m256d pb = _mm256_permute2f128_pd(a, b, 0x31);    // L1 R1 L3 R3

		__m256d left = _mm256_shuffle_pd(pa, pb, 0x0);     // L0 L1 L2 L3
		__m256d right = _mm256_shuffle_pd(pa, pb, 0xF);     // R0 R1 R2 R3

		_mm256_storeu_pd(L + i, left);
		_mm256_storeu_pd(R + i, right);
	}
	if (i < nframes) {
		deinterleave_stereo_scalar(src + 2u * i, L + i, R + i, nframes - i);
	}
}

// Interleave:
// L=[L0 L1 L2 L3], R=[R0 R1 R2 R3]
// lo = unpacklo(L,R) -> [L0 R0 L1 R1]
// hi = unpackhi(L,R) -> [L2 R2 L3 R3]
// store lo then hi
static inline void interleave_stereo_avx(const double* __restrict L, const double* __restrict R, double* dst, unsigned nframes) {
	const unsigned step = 4;
	const unsigned n4 = (nframes / step) * step;

	unsigned i = 0;
	for (; i < n4; i += step) {
		__m256d l = _mm256_loadu_pd(L + i); // [L0 L1 L2 L3]
		__m256d r = _mm256_loadu_pd(R + i); // [R0 R1 R2 R3]


		__m256d t0 = _mm256_unpacklo_pd(l, r); // [L0 R0 L2 R2]
		__m256d t1 = _mm256_unpackhi_pd(l, r); // [L1 R1 L3 R3]


		__m256d lo = _mm256_permute2f128_pd(t0, t1, 0x20); // [L0 R0 L1 R1]
		__m256d hi = _mm256_permute2f128_pd(t0, t1, 0x31); // [L2 R2 L3 R3]


		_mm256_storeu_pd(dst + (size_t)2 * i + 0, lo);
		_mm256_storeu_pd(dst + (size_t)2 * i + 4, hi);
	}
	if (i < nframes) {
		interleave_stereo_scalar(L + i, R + i, dst + 2u * i, nframes - i);
	}
}
#endif // __AVX2__ || __AVX__

FilterConfiguration::FilterConfiguration(FilterEngine* engine, const vector<FilterInfo*>& filterInfos, unsigned allChannelCount)
{
	this->allChannelCount = allChannelCount;
	realChannelCount = engine->getRealChannelCount();
	outputChannelCount = engine->getOutputChannelCount();
	unsigned maxFrameCount = engine->getMaxFrameCount();

	allSamples = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	for (size_t i = 0; i < allChannelCount; i++)
		allSamples[i] = (double*)MemoryHelper::alloc(maxFrameCount * sizeof(double));
	allSamples2 = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	for (size_t i = 0; i < allChannelCount; i++)
		allSamples2[i] = (double*)MemoryHelper::alloc(maxFrameCount * sizeof(double));
	currentSamples = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));
	currentSamples2 = (double**)MemoryHelper::alloc(allChannelCount * sizeof(double*));

	filterCount = (unsigned)filterInfos.size();
	this->filterInfos = (FilterInfo**)MemoryHelper::alloc(filterCount * sizeof(FilterInfo*));
	for (size_t i = 0; i < filterCount; i++)
		this->filterInfos[i] = filterInfos[i];
}

FilterConfiguration::~FilterConfiguration()
{
	MemoryHelper::free(currentSamples2);
	MemoryHelper::free(currentSamples);

	for (size_t i = 0; i < allChannelCount; i++)
		MemoryHelper::free(allSamples2[i]);
	MemoryHelper::free(allSamples2);

	for (size_t i = 0; i < allChannelCount; i++)
		MemoryHelper::free(allSamples[i]);
	MemoryHelper::free(allSamples);

	for (size_t i = 0; i < filterCount; i++)
	{
		filterInfos[i]->filter->~IFilter();
		MemoryHelper::free(filterInfos[i]->filter);
		if (filterInfos[i]->inChannels != NULL)
			MemoryHelper::free(filterInfos[i]->inChannels);
		if (filterInfos[i]->outChannels != NULL)
			MemoryHelper::free(filterInfos[i]->outChannels);
		MemoryHelper::free(filterInfos[i]);
	}
	MemoryHelper::free(filterInfos);
}

#pragma AVRT_CODE_BEGIN
void FilterConfiguration::read(double* input, unsigned frameCount)
{
#define DEINTERLEAVE_MACRO(ccount)\
	{\
		for (size_t c = 0; c < ccount; c++)\
		{\
			double* sampleChannel = allSamples[c];\
			double* i2 = input + c;\
			for (size_t i = 0; i < frameCount; i++)\
			{\
				sampleChannel[i] = i2[i * ccount];\
			}\
		}\
	}

	switch (realChannelCount)
	{
	case 1:
		DEINTERLEAVE_MACRO(1)
			break;
	case 2:
	{
		double* L = allSamples[0];
		double* R = allSamples[1];
		const double* src = input;

#if defined(__AVX512F__)
		deinterleave_stereo_avx512(src, L, R, frameCount);
#elif defined(__AVX2__) || defined(__AVX__)
		deinterleave_stereo_avx(src, L, R, frameCount);
#else
		deinterleave_stereo_scalar(src, L, R, frameCount);
#endif
		break;
	}
	case 6:
		DEINTERLEAVE_MACRO(6)
			break;
	case 8:
		DEINTERLEAVE_MACRO(8)
			break;
	default:
		DEINTERLEAVE_MACRO(realChannelCount)
	}
}

void FilterConfiguration::read(double** input, unsigned frameCount)
{
	for (unsigned c = 0; c < realChannelCount; c++)
		memcpy(allSamples[c], input[c], frameCount * sizeof(double));
}

void FilterConfiguration::process(unsigned frameCount)
{
	for (unsigned c = realChannelCount; c < allChannelCount; c++)
		memset(allSamples[c], 0, frameCount * sizeof(double));

	// for real mono input and >= stereo output, upmix to stereo as the Windows audio system would do automatically if no APO was present
	if (realChannelCount == 1 && outputChannelCount >= 2)
		memcpy(allSamples[1], allSamples[0], frameCount * sizeof(double));

	for (size_t i = 0; i < filterCount; i++)
	{
		FilterInfo* filterInfo = filterInfos[i];
		for (size_t j = 0; j < filterInfo->inChannelCount; j++)
			currentSamples[j] = allSamples[filterInfo->inChannels[j]];
		if (filterInfo->inPlace)
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				currentSamples2[j] = allSamples[filterInfo->outChannels[j]];
		}
		else
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				currentSamples2[j] = allSamples2[filterInfo->outChannels[j]];
		}

		filterInfo->filter->process(currentSamples2, currentSamples, frameCount);

		if (!filterInfo->inPlace)
		{
			for (size_t j = 0; j < filterInfo->outChannelCount; j++)
				swap(allSamples[filterInfo->outChannels[j]], allSamples2[filterInfo->outChannels[j]]);
			swap(currentSamples, currentSamples2);
		}
	}
}

unsigned FilterConfiguration::doTransition(FilterConfiguration* nextConfig, unsigned frameCount, unsigned transitionCounter, unsigned transitionLength)
{
	double** currentSamples = allSamples;
	double** nextSamples = nextConfig->allSamples;

	for (unsigned f = 0; f < frameCount; f++)
	{
		double factor = 0.5f * (1.0f - cos(transitionCounter * (double)M_PI / transitionLength));
		if (transitionCounter >= transitionLength)
			factor = 1.0f;

		for (unsigned c = 0; c < outputChannelCount; c++)
			currentSamples[c][f] = currentSamples[c][f] * (1 - factor) + nextSamples[c][f] * factor;

		transitionCounter++;
	}

	return transitionCounter;
}

void FilterConfiguration::write(double* output, unsigned frameCount)
{
#define INTERLEAVE_MACRO(ccount)\
	for (size_t c = 0; c < ccount; c++)\
	{\
		double* sampleChannel = allSamples[c];\
		double* o2 = output + c;\
		for (unsigned i = 0; i < frameCount; i++)\
		{\
			o2[i * ccount] = sampleChannel[i];\
		}\
	}

	switch (outputChannelCount)
	{
	case 1:
		INTERLEAVE_MACRO(1)
			break;
	case 2:
	{
		const double* L = allSamples[0];
		const double* R = allSamples[1];
		double* dst = output;

#if defined(__AVX512F__)
		interleave_stereo_avx512(L, R, dst, frameCount);
#elif defined(__AVX2__) || defined(__AVX__)
		interleave_stereo_avx(L, R, dst, frameCount);
#else
		interleave_stereo_scalar(L, R, dst, frameCount);
#endif
		break;
	}
	case 6:
		INTERLEAVE_MACRO(6)
			break;
	case 8:
		INTERLEAVE_MACRO(8)
			break;
	default:
		INTERLEAVE_MACRO(outputChannelCount)
	}
}

void FilterConfiguration::write(double** output, unsigned frameCount)
{
	for (unsigned i = 0; i < outputChannelCount; i++)
		memcpy(output[i], allSamples[i], frameCount * sizeof(double));
}
#pragma AVRT_CODE_END

bool FilterConfiguration::isEmpty()
{
	return filterCount == 0;
}