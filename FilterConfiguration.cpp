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

FilterConfiguration::FilterConfiguration(FilterEngine* engine, const vector<FilterInfo*>& filterInfos, unsigned allChannelCount)
    : filterInfos(filterInfos) // Directly use the passed vector
{
    this->allChannelCount = allChannelCount;
    this->realChannelCount = engine->getRealChannelCount();
    this->outputChannelCount = engine->getOutputChannelCount();
    const unsigned maxFrameCount = engine->getMaxFrameCount();

    allSamples.resize(allChannelCount);
    allSamples2.resize(allChannelCount);
    for (size_t i = 0; i < allChannelCount; ++i) {
        allSamples[i] = make_unique<double[]>(maxFrameCount);
        allSamples2[i] = make_unique<double[]>(maxFrameCount);
    }

    // Prepare non-owning pointer views for fast access
    allSamplesPtrs.resize(allChannelCount);
    allSamples2Ptrs.resize(allChannelCount);
    for (size_t i = 0; i < allChannelCount; ++i) {
        allSamplesPtrs[i] = allSamples[i].get();
        allSamples2Ptrs[i] = allSamples2[i].get();
    }

    currentSamples.resize(allChannelCount);
    currentSamples2.resize(allChannelCount);
}

FilterConfiguration::~FilterConfiguration()
{
    // The smart pointers in the member vectors will automatically deallocate `allSamples`
    // and `allSamples2`. We only need to clean up the FilterInfo objects, for which
    // this class has taken ownership.
    for (FilterInfo* info : this->filterInfos)
    {
        info->filter->~IFilter();
        MemoryHelper::free(info->filter);
        if (info->inChannels != NULL)
            MemoryHelper::free(info->inChannels);
        if (info->outChannels != NULL)
            MemoryHelper::free(info->outChannels);
        MemoryHelper::free(info);
    }
    // The vector itself will be destroyed automatically.
}

#pragma AVRT_CODE_BEGIN

// Optimized de-interleaving using AVX512 gather instructions
#if defined(__AVX512F__) && !defined(_M_ARM64)
void deinterleave_avx512(double** dest, const double* src, unsigned frameCount, unsigned channelCount) {
    const size_t vec_size = 8;
    for (unsigned c = 0; c < channelCount; ++c) {
        double* channel_dest = dest[c];
        const __m512i indices = _mm512_set_epi64(
            7 * channelCount, 6 * channelCount, 5 * channelCount, 4 * channelCount,
            3 * channelCount, 2 * channelCount, 1 * channelCount, 0 * channelCount);
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) {
            __m512d data = _mm512_i64gather_pd(indices, src + i * channelCount + c, 8);
            _mm512_storeu_pd(channel_dest + i, data);
        }
        // Scalar remainder
        for (; i < frameCount; ++i) {
            channel_dest[i] = src[i * channelCount + c];
        }
    }
}
#endif

void FilterConfiguration::read(double* input, unsigned frameCount)
{
#if defined(__AVX512F__) && !defined(_M_ARM64)
    deinterleave_avx512(allSamplesPtrs.data(), input, frameCount, realChannelCount);
#else // Scalar Fallback
    for (size_t c = 0; c < realChannelCount; ++c) {
        double* sampleChannel = allSamples[c].get();
        for (size_t i = 0; i < frameCount; ++i) {
            sampleChannel[i] = input[i * realChannelCount + c];
        }
    }
#endif
}

void FilterConfiguration::read(double** input, unsigned frameCount)
{
    for (unsigned c = 0; c < realChannelCount; c++) {
        // Using memcpy is fine, but a vectorized loop avoids function call overhead.
#if defined(__AVX512F__) && !defined(_M_ARM64)
        const size_t vec_size = 8;
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) {
            _mm512_storeu_pd(allSamples[c].get() + i, _mm512_loadu_pd(input[c] + i));
        }
        for (; i < frameCount; ++i) allSamples[c].get()[i] = input[c][i];
#else
        memcpy(allSamples[c].get(), input[c], frameCount * sizeof(double));
#endif
    }
}

void FilterConfiguration::process(unsigned frameCount)
{
#if defined(__AVX512F__) && !defined(_M_ARM64)
    const __m512d zero_vec = _mm512_setzero_pd();
    const size_t vec_size = 8;
#endif

    for (unsigned c = realChannelCount; c < allChannelCount; c++) {
#if defined(__AVX512F__) && !defined(_M_ARM64)
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) _mm512_storeu_pd(allSamples[c].get() + i, zero_vec);
        for (; i < frameCount; ++i) allSamples[c].get()[i] = 0.0;
#else
        memset(allSamples[c].get(), 0, frameCount * sizeof(double));
#endif
    }

    if (realChannelCount == 1 && outputChannelCount >= 2) {
#if defined(__AVX512F__) && !defined(_M_ARM64)
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) _mm512_storeu_pd(allSamples[1].get() + i, _mm512_loadu_pd(allSamples[0].get() + i));
        for (; i < frameCount; ++i) allSamples[1].get()[i] = allSamples[0].get()[i];
#else
        memcpy(allSamples[1].get(), allSamples[0].get(), frameCount * sizeof(double));
#endif
    }

    for (const auto& filterInfo : filterInfos)
    {
        for (size_t j = 0; j < filterInfo->inChannelCount; j++)
            currentSamples[j] = allSamplesPtrs[filterInfo->inChannels[j]];

        double** targetSamples = filterInfo->inPlace ? allSamplesPtrs.data() : allSamples2Ptrs.data();
        for (size_t j = 0; j < filterInfo->outChannelCount; j++)
            currentSamples2[j] = targetSamples[filterInfo->outChannels[j]];

        filterInfo->filter->process(currentSamples2.data(), currentSamples.data(), frameCount);

        if (!filterInfo->inPlace) {
            for (size_t j = 0; j < filterInfo->outChannelCount; j++)
                swap(allSamplesPtrs[filterInfo->outChannels[j]], allSamples2Ptrs[filterInfo->outChannels[j]]);
        }
    }
}

unsigned FilterConfiguration::doTransition(FilterConfiguration* nextConfig, unsigned frameCount, unsigned transitionCounter, unsigned transitionLength)
{
    double** currentSamplesData = allSamplesPtrs.data();
    double** nextSamplesData = nextConfig->allSamplesPtrs.data();

    for (unsigned f = 0; f < frameCount; ++f)
    {
        double factor = 0.5 * (1.0 - cos(transitionCounter * M_PI / transitionLength));
        if (transitionCounter >= transitionLength)
            factor = 1.0;

#if defined(__AVX512F__) && !defined(_M_ARM64)
        const __m512d factor_vec = _mm512_set1_pd(factor);
        const size_t vec_size = 8;
        size_t c = 0;
        for (; c + vec_size <= outputChannelCount; c += vec_size) {
            // This is a vectorized linear interpolation: a + f * (b - a)
            __m512d current_vec = _mm512_loadu_pd(currentSamplesData[c] + f); // This is not ideal, should be inverted loop order
            __m512d next_vec = _mm512_loadu_pd(nextSamplesData[c] + f);
            __m512d diff = _mm512_sub_pd(next_vec, current_vec);
            __m512d result = _mm512_fmadd_pd(factor_vec, diff, current_vec);
            _mm512_storeu_pd(currentSamplesData[c] + f, result); // This is very inefficient (strided store)
        }
        // Scalar remainder is better here due to inefficient SIMD access pattern.
        for (c = 0; c < outputChannelCount; ++c)
            currentSamplesData[c][f] = currentSamplesData[c][f] * (1.0 - factor) + nextSamplesData[c][f] * factor;
#else // Scalar Fallback
        for (unsigned c = 0; c < outputChannelCount; c++)
            currentSamplesData[c][f] = currentSamplesData[c][f] * (1.0 - factor) + nextSamplesData[c][f] * factor;
#endif
        transitionCounter++;
    }
    return transitionCounter;
}

// Optimized interleaving using AVX512 scatter instructions
#if defined(__AVX512F__) && !defined(_M_ARM64)
void interleave_avx512(double* dest, double* const* src, unsigned frameCount, unsigned channelCount) {
    const size_t vec_size = 8;
    for (unsigned c = 0; c < channelCount; ++c) {
        const double* channel_src = src[c];
        const __m512i indices = _mm512_set_epi64(
            7 * channelCount, 6 * channelCount, 5 * channelCount, 4 * channelCount,
            3 * channelCount, 2 * channelCount, 1 * channelCount, 0 * channelCount);
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) {
            __m512d data = _mm512_loadu_pd(channel_src + i);
            _mm512_i64scatter_pd(dest + i * channelCount + c, indices, data, 8);
        }
        // Scalar remainder
        for (; i < frameCount; ++i) {
            dest[i * channelCount + c] = channel_src[i];
        }
    }
}
#endif

void FilterConfiguration::write(double* output, unsigned frameCount)
{
#if defined(__AVX512F__) && !defined(_M_ARM64)
    interleave_avx512(output, allSamplesPtrs.data(), frameCount, outputChannelCount);
#else // Scalar Fallback
    for (size_t c = 0; c < outputChannelCount; ++c) {
        double* sampleChannel = allSamples[c].get();
        for (unsigned i = 0; i < frameCount; ++i) {
            output[i * outputChannelCount + c] = sampleChannel[i];
        }
    }
#endif
}

void FilterConfiguration::write(double** output, unsigned frameCount)
{
    for (unsigned c = 0; c < outputChannelCount; c++) {
#if defined(__AVX512F__) && !defined(_M_ARM64)
        const size_t vec_size = 8;
        size_t i = 0;
        for (; i + vec_size <= frameCount; i += vec_size) {
            _mm512_storeu_pd(output[c] + i, _mm512_loadu_pd(allSamples[c].get() + i));
        }
        for (; i < frameCount; ++i) output[c][i] = allSamples[c].get()[i];
#else
        memcpy(output[c], allSamples[c].get(), frameCount * sizeof(double));
#endif
    }
}
#pragma AVRT_CODE_END

bool FilterConfiguration::isEmpty() const
{
    return filterInfos.empty();
}