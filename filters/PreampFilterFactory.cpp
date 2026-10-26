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
#define _USE_MATH_DEFINES
#include <cmath>
#ifndef _M_ARM64
#include <immintrin.h>
#endif

#include "PreampFilter.h"

using namespace std;

PreampFilter::PreampFilter(double dbGain, bool removeDcOffset)
	: dbGain(std::max(-100.0, std::min(100.0, dbGain))), removeDcOffset(removeDcOffset), gain(0.0), channelCount(0)
{
	gain = pow(10.0, this->dbGain / 20.0);
}

vector<wstring> PreampFilter::initialize(float sampleRate, unsigned maxFrameCount, vector<wstring> channelNames)
{
	this->channelCount = channelNames.size();
	return channelNames;
}

#pragma AVRT_CODE_BEGIN
void PreampFilter::process(double** output, double** input, unsigned frameCount)
{
	const double gainFactor = this->gain;

	for (size_t c = 0; c < channelCount; ++c)
	{
		double* inputChannel = input[c];
		double* outputChannel = output[c];
		size_t i = 0;

		if (removeDcOffset && frameCount > 0)
		{
			double sum = 0.0;
			for (unsigned j = 0; j < frameCount; ++j)
				sum += inputChannel[j];
			double dc = sum / static_cast<double>(frameCount);
			for (; i < frameCount; ++i)
				outputChannel[i] = (inputChannel[i] - dc) * gainFactor;
			continue;
		}

#if defined(__AVX512F__) && !defined(_M_ARM64)
		{
			const size_t simd_width = 8;
			if (frameCount >= simd_width) {
				const __m512d gain_vec = _mm512_set1_pd(gainFactor);
				for (; i + simd_width <= frameCount; i += simd_width)
				{
					const __m512d samples = _mm512_loadu_pd(inputChannel + i);
					const __m512d result = _mm512_mul_pd(samples, gain_vec);
					_mm512_storeu_pd(outputChannel + i, result);
				}
			}
		}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
		{
			const size_t simd_width = 4;
			if (frameCount - i >= simd_width) {
				const __m256d gain_vec = _mm256_set1_pd(gainFactor);
				for (; i + simd_width <= frameCount; i += simd_width)
				{
					const __m256d samples = _mm256_loadu_pd(inputChannel + i);
					const __m256d result = _mm256_mul_pd(samples, gain_vec);
					_mm256_storeu_pd(outputChannel + i, result);
				}
			}
		}
#endif

#if !defined(_M_ARM64)
		{
			const size_t simd_width = 2;
			if (frameCount - i >= simd_width) {
				const __m128d gain_vec = _mm_set1_pd(gainFactor);
				for (; i + simd_width <= frameCount; i += simd_width)
				{
					const __m128d samples = _mm_loadu_pd(inputChannel + i);
					const __m128d result = _mm_mul_pd(samples, gain_vec);
					_mm_storeu_pd(outputChannel + i, result);
				}
			}
		}
#endif

		for (; i < frameCount; ++i)
		{
			outputChannel[i] = inputChannel[i] * gainFactor;
		}
	}
}
#pragma AVRT_CODE_END
