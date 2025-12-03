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
#include "BiQuadFilter.h"
#ifndef _M_ARM64
#include <immintrin.h>
#endif

BiQuadFilter::BiQuadFilter(BiQuad::Type type, double dbGain, double freq, double bandwidthOrQOrS, bool isBandwidthOrS, bool isCornerFreq)
    : type(type), dbGain(dbGain), freq(freq), bandwidthOrQOrS(bandwidthOrQOrS), isBandwidthOrS(isBandwidthOrS), isCornerFreq(isCornerFreq), channelCount(0)
{
}

BiQuad::Type BiQuadFilter::getType() const
{
    return type;
}

double BiQuadFilter::getDbGain() const
{
    return dbGain;
}

double BiQuadFilter::getFreq() const
{
    return freq;
}

double BiQuadFilter::getBandwidthOrQOrS() const
{
    return bandwidthOrQOrS;
}

bool BiQuadFilter::getIsBandwidthOrS() const
{
    return isBandwidthOrS;
}

bool BiQuadFilter::getIsCornerFreq() const
{
    return isCornerFreq;
}

std::vector<std::wstring> BiQuadFilter::initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames)
{
    this->channelCount = channelNames.size();

    // Resize SoA vectors
    a0.resize(channelCount); a1.resize(channelCount); a2.resize(channelCount);
    b1.resize(channelCount); b2.resize(channelCount);
    x1.assign(channelCount, 0.0); x2.assign(channelCount, 0.0);
    y1.assign(channelCount, 0.0); y2.assign(channelCount, 0.0);

    double biquadFreq = freq;
    if (isCornerFreq && (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF))
    {
        double s = bandwidthOrQOrS;
        if (!isBandwidthOrS) // Q
        {
            double q = bandwidthOrQOrS;
            double a = pow(10, dbGain / 40);
            s = 1.0 / ((1.0 / (q * q) - 2.0) / (a + 1.0 / a) + 1.0);
        }
        double centerFreqFactor = pow(10.0, abs(dbGain) / 80.0 / s);
        if (type == BiQuad::LOW_SHELF)
            biquadFreq *= centerFreqFactor;
        else
            biquadFreq /= centerFreqFactor;
    }

    // 1. Create a single master BiQuad to perform the coefficient calculation.
    BiQuad masterBiquad(type, dbGain, biquadFreq, sampleRate, bandwidthOrQOrS, isBandwidthOrS);

    // 2. Prepare variables to receive the coefficients.
    double temp_a0;
    double temp_coeffs[4];

    // 3. Call public method to get the coefficients
    masterBiquad.getCoefficients(temp_coeffs, temp_a0);

    // 4. Populate our SoA vectors with the coefficients for all channels.
    // The mapping is based on the original `process` function's variable usage:
    // result = a0*sample + a[1]*x2 + a[0]*x1 - a[3]*y2 - a[2]*y1;
    // Textbook: result = b0*sample + b2*x2 + b1*x1 - a2*y2 - a1*y1
    const double coeff_b1 = temp_coeffs[0];
    const double coeff_b2 = temp_coeffs[1];
    const double coeff_a1 = temp_coeffs[2];
    const double coeff_a2 = temp_coeffs[3];

    for (unsigned i = 0; i < channelCount; ++i)
    {
        this->a0[i] = temp_a0;      // Corresponds to b0
        this->b1[i] = coeff_b1;     // Corresponds to b1
        this->b2[i] = coeff_b2;     // Corresponds to b2
        this->a1[i] = coeff_a1;     // Corresponds to a1
        this->a2[i] = coeff_a2;     // Corresponds to a2
    }

    return channelNames;
}

#pragma AVRT_CODE_BEGIN

void BiQuadFilter::process(double** output, double** input, unsigned frameCount)
{
    unsigned old_mxcsr = _mm_getcsr();
    _mm_setcsr(old_mxcsr | 0x8040);

    unsigned processedChannels = 0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
    const unsigned avx512_width = 8;
    unsigned num_avx512_chunks = ((unsigned)channelCount - processedChannels) / avx512_width;
    if (num_avx512_chunks > 0) {
        process_avx512(output, input, frameCount, processedChannels, num_avx512_chunks * avx512_width);
        processedChannels += num_avx512_chunks * avx512_width;
    }
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
    const unsigned avx256_width = 4;
    unsigned num_avx256_chunks = ((unsigned)channelCount - processedChannels) / avx256_width;
    if (num_avx256_chunks > 0) {
        process_avx256(output, input, frameCount, processedChannels, num_avx256_chunks * avx256_width);
        processedChannels += num_avx256_chunks * avx256_width;
    }
#endif

    const unsigned sse128_width = 2;
    unsigned num_sse128_chunks = ((unsigned)channelCount - processedChannels) / sse128_width;
    if (num_sse128_chunks > 0) {
        process_sse128(output, input, frameCount, processedChannels, num_sse128_chunks * sse128_width);
        processedChannels += num_sse128_chunks * sse128_width;
    }

    if (processedChannels < (unsigned)channelCount) {
        process_scalar(output, input, frameCount, processedChannels);
    }

    _mm_setcsr(old_mxcsr);
}


#if defined(__AVX512F__) && !defined(_M_ARM64)
// AVX-512 optimized processing for 8 channels at a time
void BiQuadFilter::process_avx512(double** output, double** input, unsigned frameCount, unsigned startChannel, unsigned numChannels)
{
    const unsigned simd_width = 8;
    for (unsigned i = startChannel; i < startChannel + numChannels; i += simd_width)
    {
        const __m512d _a0 = _mm512_load_pd(&a0[i]);
        const __m512d _b1 = _mm512_load_pd(&b1[i]);
        const __m512d _b2 = _mm512_load_pd(&b2[i]);
        const __m512d _a1 = _mm512_load_pd(&a1[i]);
        const __m512d _a2 = _mm512_load_pd(&a2[i]);

        __m512d _x1 = _mm512_load_pd(&x1[i]);
        __m512d _x2 = _mm512_load_pd(&x2[i]);
        __m512d _y1 = _mm512_load_pd(&y1[i]);
        __m512d _y2 = _mm512_load_pd(&y2[i]);

        for (unsigned j = 0; j < frameCount; ++j)
        {
            __m512d _sample = _mm512_set_pd(input[i + 7][j], input[i + 6][j], input[i + 5][j], input[i + 4][j],
                input[i + 3][j], input[i + 2][j], input[i + 1][j], input[i + 0][j]);

            __m512d result = _mm512_mul_pd(_a0, _sample);
            result = _mm512_fmadd_pd(_b1, _x1, result);
            result = _mm512_fmadd_pd(_b2, _x2, result);
            result = _mm512_fnmadd_pd(_a1, _y1, result);
            result = _mm512_fnmadd_pd(_a2, _y2, result);

            _x2 = _x1; _x1 = _sample;
            _y2 = _y1; _y1 = result;

            double result_array[8]; // Use a temporary array for the scatter operation
            _mm512_storeu_pd(result_array, result);
            for (int k = 0; k < 8; ++k) output[i + k][j] = result_array[k];
        }

        _mm512_store_pd(&x1[i], _x1);
        _mm512_store_pd(&x2[i], _x2);
        _mm512_store_pd(&y1[i], _y1);
        _mm512_store_pd(&y2[i], _y2);
    }
}
#endif

#if defined(__AVX2__) && !defined(_M_ARM64)
// AVX2 optimized processing for 4 channels at a time
void BiQuadFilter::process_avx256(double** output, double** input, unsigned frameCount, unsigned startChannel, unsigned numChannels)
{
    const unsigned simd_width = 4;
    for (unsigned i = startChannel; i < startChannel + numChannels; i += simd_width)
    {
        const __m256d _a0 = _mm256_load_pd(&a0[i]);
        const __m256d _b1 = _mm256_load_pd(&b1[i]);
        const __m256d _b2 = _mm256_load_pd(&b2[i]);
        const __m256d _a1 = _mm256_load_pd(&a1[i]);
        const __m256d _a2 = _mm256_load_pd(&a2[i]);

        __m256d _x1 = _mm256_load_pd(&x1[i]);
        __m256d _x2 = _mm256_load_pd(&x2[i]);
        __m256d _y1 = _mm256_load_pd(&y1[i]);
        __m256d _y2 = _mm256_load_pd(&y2[i]);

        for (unsigned j = 0; j < frameCount; ++j)
        {
            __m256d _sample = _mm256_set_pd(input[i + 3][j], input[i + 2][j], input[i + 1][j], input[i + 0][j]);

            __m256d result = _mm256_mul_pd(_a0, _sample);
            result = _mm256_fmadd_pd(_b1, _x1, result);
            result = _mm256_fmadd_pd(_b2, _x2, result);
            result = _mm256_fnmadd_pd(_a1, _y1, result);
            result = _mm256_fnmadd_pd(_a2, _y2, result);

            _x2 = _x1; _x1 = _sample;
            _y2 = _y1; _y1 = result;

            double result_array[4];
            _mm256_storeu_pd(result_array, result);
            output[i + 0][j] = result_array[0];
            output[i + 1][j] = result_array[1];
            output[i + 2][j] = result_array[2];
            output[i + 3][j] = result_array[3];
        }

        _mm256_store_pd(&x1[i], _x1);
        _mm256_store_pd(&x2[i], _x2);
        _mm256_store_pd(&y1[i], _y1);
        _mm256_store_pd(&y2[i], _y2);
    }
}
#endif

// SSE optimized processing for 2 channels (stereo) at a time
void BiQuadFilter::process_sse128(double** output, double** input, unsigned frameCount, unsigned startChannel, unsigned numChannels)
{
    const unsigned simd_width = 2;
    for (unsigned i = startChannel; i < startChannel + numChannels; i += simd_width)
    {
        const __m128d _a0 = _mm_load_pd(&a0[i]);
        const __m128d _b1 = _mm_load_pd(&b1[i]);
        const __m128d _b2 = _mm_load_pd(&b2[i]);
        const __m128d _a1 = _mm_load_pd(&a1[i]);
        const __m128d _a2 = _mm_load_pd(&a2[i]);

        __m128d _x1 = _mm_load_pd(&x1[i]);
        __m128d _x2 = _mm_load_pd(&x2[i]);
        __m128d _y1 = _mm_load_pd(&y1[i]);
        __m128d _y2 = _mm_load_pd(&y2[i]);

        for (unsigned j = 0; j < frameCount; ++j)
        {
            __m128d _sample = _mm_set_pd(input[i + 1][j], input[i + 0][j]);

            // Use FMA if available (AVX+), otherwise it will fallback to mul/add sequence with SSE2
#if defined(__AVX2__)
            __m128d result = _mm_mul_pd(_a0, _sample);
            result = _mm_fmadd_pd(_b1, _x1, result);
            result = _mm_fmadd_pd(_b2, _x2, result);
            result = _mm_fnmadd_pd(_a1, _y1, result);
            result = _mm_fnmadd_pd(_a2, _y2, result);
#else // Fallback for pure SSE2 CPUs (no FMA)
            __m128d term1 = _mm_mul_pd(_a0, _sample);
            __m128d term2 = _mm_mul_pd(_b1, _x1);
            __m128d term3 = _mm_mul_pd(_b2, _x2);
            __m128d term4 = _mm_mul_pd(_a1, _y1);
            __m128d term5 = _mm_mul_pd(_a2, _y2);
            __m128d result = _mm_add_pd(term1, term2);
            result = _mm_add_pd(result, term3);
            result = _mm_sub_pd(result, term4);
            result = _mm_sub_pd(result, term5);
#endif
            _x2 = _x1; _x1 = _sample;
            _y2 = _y1; _y1 = result;

            double result_array[2];
            _mm_storeu_pd(result_array, result);
            output[i + 0][j] = result_array[0];
            output[i + 1][j] = result_array[1];
        }
        _mm_store_pd(&x1[i], _x1);
        _mm_store_pd(&x2[i], _x2);
        _mm_store_pd(&y1[i], _y1);
        _mm_store_pd(&y2[i], _y2);
    }
}


// Scalar processing for any final leftover channels (e.g., the 7th channel in a 7.1 setup)
void BiQuadFilter::process_scalar(double** output, double** input, unsigned frameCount, unsigned startChannel)
{
    for (unsigned i = startChannel; i < channelCount; ++i)
    {
        double cur_x1 = x1[i], cur_x2 = x2[i];
        double cur_y1 = y1[i], cur_y2 = y2[i];
        const double c_a0 = a0[i], c_b1 = b1[i], c_b2 = b2[i];
        const double c_a1 = a1[i], c_a2 = a2[i];
        double* inputChannel = input[i];
        double* outputChannel = output[i];
        for (unsigned j = 0; j < frameCount; ++j)
        {
            double sample = inputChannel[j];
            double result = c_a0 * sample + c_b1 * cur_x1 + c_b2 * cur_x2 - c_a1 * cur_y1 - c_a2 * cur_y2;
            cur_x2 = cur_x1;
            cur_x1 = sample;
            cur_y2 = cur_y1;
            cur_y1 = result;
            outputChannel[j] = result;
        }
        x1[i] = cur_x1; x2[i] = cur_x2;
        y1[i] = cur_y1; y2[i] = cur_y2;
    }
}
#pragma AVRT_CODE_END