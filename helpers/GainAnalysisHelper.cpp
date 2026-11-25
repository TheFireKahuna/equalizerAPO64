/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2026

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
#include <algorithm>
#include <limits>
#include <fftw3.h>

#include "helpers/GainAnalysisHelper.h"
#include "filters/PreampFilter.h"
#include "filters/BiQuadFilter.h"
#include "filters/GraphicEQFilter.h"

using namespace std;

namespace
{
	struct BiQuadEvalItem
	{
		BiQuad biquad;
		double centerFreqHz;
		double effectiveQ;
	};

	struct GraphicEQEvalItem
	{
		unsigned fftSize;
		vector<double> binMagLinear;
		vector<double> nodeFreqs;
	};

	void sortAndDeduplicate(vector<double>& freqs, double minDelta)
	{
		sort(freqs.begin(), freqs.end());
		size_t writeIdx = 0;
		for (size_t i = 0; i < freqs.size(); ++i)
		{
			if (writeIdx == 0 || (freqs[i] - freqs[writeIdx - 1]) > minDelta)
				freqs[writeIdx++] = freqs[i];
		}
		freqs.resize(writeIdx);
	}
}

double GainAnalysisHelper::analyzeChain(
	const vector<FilterInfo*>& filterInfos,
	double sampleRate,
	vector<FilterNode>* outPlotNodes)
{
	double safeSampleRate = (sampleRate > 100.0) ? sampleRate : 48000.0;
	double maxEvalFreq = 0.4999 * safeSampleRate;

	if (filterInfos.empty())
	{
		if (outPlotNodes != nullptr)
		{
			outPlotNodes->clear();
			outPlotNodes->push_back(FilterNode(0.001, 0.0));
			outPlotNodes->push_back(FilterNode(1.0, 0.0));
			outPlotNodes->push_back(FilterNode(maxEvalFreq, 0.0));
		}
		return 0.0;
	}

	double totalPreampDb = 0.0;
	vector<BiQuadEvalItem> biquads;
	vector<GraphicEQEvalItem> geqs;

	for (const FilterInfo* info : filterInfos)
	{
		if (info == nullptr || info->filter == nullptr)
			continue;

		const IFilter* filter = info->filter;
		switch (filter->getFilterKind())
		{
		case IFilter::FilterKind::Preamp:
		{
			const PreampFilter* pf = static_cast<const PreampFilter*>(filter);
			totalPreampDb += pf->getDbGain();
			break;
		}
		case IFilter::FilterKind::BiQuad:
		{
			const BiQuadFilter* bqf = static_cast<const BiQuadFilter*>(filter);
			double fc = 100.0;
			double effQ = 1.0;
			BiQuad bq = bqf->createValidatedBiQuad(safeSampleRate, &fc, &effQ);
			biquads.push_back({ bq, fc, effQ });
			break;
		}
		case IFilter::FilterKind::GraphicEQ:
		{
			const GraphicEQFilter* geqf = static_cast<const GraphicEQFilter*>(filter);
			vector<double> ir;
			geqf->computeImpulseResponse(safeSampleRate, ir);

			unsigned fftSize = max<unsigned>(65536u, geqf->getFilterLength() * 4u);
			fftw_make_planner_thread_safe();
			double* timeBuf = fftw_alloc_real(fftSize);
			fftw_complex* freqBuf = fftw_alloc_complex(fftSize / 2 + 1);
			fftw_plan plan = fftw_plan_dft_r2c_1d((int)fftSize, timeBuf, freqBuf, FFTW_ESTIMATE);

			memset(timeBuf, 0, fftSize * sizeof(double));
			size_t copyCount = min<size_t>(ir.size(), fftSize);
			if (copyCount > 0)
				memcpy(timeBuf, ir.data(), copyCount * sizeof(double));

			fftw_execute(plan);

			GraphicEQEvalItem item;
			item.fftSize = fftSize;
			unsigned binCount = fftSize / 2 + 1;
			item.binMagLinear.resize(binCount);
			for (unsigned k = 0; k < binCount; ++k)
			{
				double re = freqBuf[k][0];
				double im = freqBuf[k][1];
				item.binMagLinear[k] = sqrt(re * re + im * im);
			}
			for (const FilterNode& node : geqf->getNodes())
				item.nodeFreqs.push_back(node.freq);

			fftw_destroy_plan(plan);
			fftw_free(freqBuf);
			fftw_free(timeBuf);

			geqs.push_back(std::move(item));
			break;
		}
		default:
			break;
		}
	}

	auto evalGainDb = [&](double freq) -> double
	{
		double f = std::clamp(freq, 0.0, maxEvalFreq);
		double sumDb = totalPreampDb;

		for (BiQuadEvalItem& item : biquads)
		{
			double g = item.biquad.gainAt(f, safeSampleRate);
			if (!std::isfinite(g))
				return numeric_limits<double>::infinity();
			sumDb += g;
		}

		for (const GraphicEQEvalItem& geq : geqs)
		{
			double binPos = f * (double)geq.fftSize / safeSampleRate;
			unsigned maxBin = geq.fftSize / 2;
			if (binPos <= 0.0)
			{
				double mag = geq.binMagLinear[0];
				sumDb += (mag > 1e-15) ? (20.0 * log10(mag)) : -300.0;
			}
			else if (binPos >= (double)maxBin)
			{
				double mag = geq.binMagLinear[maxBin];
				sumDb += (mag > 1e-15) ? (20.0 * log10(mag)) : -300.0;
			}
			else
			{
				unsigned idx = (unsigned)binPos;
				double frac = binPos - (double)idx;
				double mag = geq.binMagLinear[idx] * (1.0 - frac) + geq.binMagLinear[idx + 1] * frac;
				sumDb += (mag > 1e-15) ? (20.0 * log10(mag)) : -300.0;
			}
		}

		return sumDb;
	};

	// Build candidate frequency grid for global peak search
	vector<double> searchFreqs;
	searchFreqs.reserve(4600 + (geqs.empty() ? 0 : 33000));

	searchFreqs.push_back(0.0);
	searchFreqs.push_back(0.25);
	searchFreqs.push_back(0.5);

	constexpr int LOG_GRID_POINTS = 4096;
	const double logMin = log(1.0);
	const double logMax = log(maxEvalFreq);
	for (int i = 0; i <= LOG_GRID_POINTS; ++i)
	{
		double t = (double)i / (double)LOG_GRID_POINTS;
		searchFreqs.push_back(exp(logMin + t * (logMax - logMin)));
	}

	// Add dense local points around every BiQuad center/resonant frequency
	for (const BiQuadEvalItem& item : biquads)
	{
		double fc = std::clamp(item.centerFreqHz, 0.5, maxEvalFreq);
		searchFreqs.push_back(fc);
		double q = max(1.0, item.effectiveQ);
		for (int k = 1; k <= 16; ++k)
		{
			double delta = (double)k / (6.0 * q);
			double fLow = fc * max(0.01, 1.0 - delta);
			double fHigh = fc * (1.0 + delta);
			if (fLow >= 0.0 && fLow <= maxEvalFreq)
				searchFreqs.push_back(fLow);
			if (fHigh >= 0.0 && fHigh <= maxEvalFreq)
				searchFreqs.push_back(fHigh);
		}
	}

	// Add GraphicEQ node frequencies and dense FFT bin frequencies
	if (!geqs.empty())
	{
		unsigned maxFft = 0;
		for (const GraphicEQEvalItem& geq : geqs)
		{
			maxFft = max(maxFft, geq.fftSize);
			for (double nf : geq.nodeFreqs)
			{
				if (nf >= 0.0 && nf <= maxEvalFreq)
					searchFreqs.push_back(nf);
			}
		}
		unsigned halfBins = maxFft / 2;
		for (unsigned k = 1; k < halfBins; ++k)
		{
			double bf = (double)k * safeSampleRate / (double)maxFft;
			if (bf <= maxEvalFreq)
				searchFreqs.push_back(bf);
		}
	}

	sortAndDeduplicate(searchFreqs, 1e-8);

	vector<double> gains(searchFreqs.size());
	double maxPeakDb = -numeric_limits<double>::infinity();
	double bestFreq = 0.0;

	for (size_t i = 0; i < searchFreqs.size(); ++i)
	{
		double g = evalGainDb(searchFreqs[i]);
		gains[i] = g;
		if (!std::isfinite(g))
			return numeric_limits<double>::infinity();
		if (g > maxPeakDb)
		{
			maxPeakDb = g;
			bestFreq = searchFreqs[i];
		}
	}

	// Refine local maxima with 28-iteration Golden-Section Search
	const double invPhi = (sqrt(5.0) - 1.0) * 0.5; // ~0.61803398875
	for (size_t i = 1; i + 1 < searchFreqs.size(); ++i)
	{
		if (gains[i] >= gains[i - 1] - 1e-7 && gains[i] >= gains[i + 1] - 1e-7)
		{
			// Only refine candidates within 2 dB of current best peak
			if (gains[i] < maxPeakDb - 2.0)
				continue;

			double a = searchFreqs[i - 1];
			double b = searchFreqs[i + 1];
			double c = b - invPhi * (b - a);
			double d = a + invPhi * (b - a);
			double fcVal = evalGainDb(c);
			double fdVal = evalGainDb(d);

			for (int iter = 0; iter < 28; ++iter)
			{
				if (fcVal >= fdVal)
				{
					b = d;
					d = c;
					fdVal = fcVal;
					c = b - invPhi * (b - a);
					fcVal = evalGainDb(c);
				}
				else
				{
					a = c;
					c = d;
					fcVal = fdVal;
					d = a + invPhi * (b - a);
					fdVal = evalGainDb(d);
				}
			}

			double mid = 0.5 * (a + b);
			double midVal = evalGainDb(mid);
			if (midVal > maxPeakDb)
			{
				maxPeakDb = midVal;
				bestFreq = mid;
			}
		}
	}

	if (outPlotNodes != nullptr)
	{
		vector<double> plotFreqs;
		plotFreqs.reserve(2600);
		plotFreqs.push_back(0.001);

		constexpr int PLOT_LOG_POINTS = 2048;
		for (int i = 0; i <= PLOT_LOG_POINTS; ++i)
		{
			double t = (double)i / (double)PLOT_LOG_POINTS;
			plotFreqs.push_back(exp(logMin + t * (logMax - logMin)));
		}

		for (const BiQuadEvalItem& item : biquads)
		{
			double fc = std::clamp(item.centerFreqHz, 1.0, maxEvalFreq);
			plotFreqs.push_back(fc);
			double q = max(1.0, item.effectiveQ);
			for (int k = 1; k <= 16; ++k)
			{
				double delta = (double)k / (8.0 * q);
				double fLow = fc * max(0.01, 1.0 - delta);
				double fHigh = fc * (1.0 + delta);
				if (fLow >= 1.0 && fLow <= maxEvalFreq)
					plotFreqs.push_back(fLow);
				if (fHigh >= 1.0 && fHigh <= maxEvalFreq)
					plotFreqs.push_back(fHigh);
			}
		}

		for (const GraphicEQEvalItem& geq : geqs)
		{
			for (double nf : geq.nodeFreqs)
			{
				if (nf >= 1.0 && nf <= maxEvalFreq)
					plotFreqs.push_back(nf);
			}
		}

		if (bestFreq >= 0.001 && bestFreq <= maxEvalFreq)
			plotFreqs.push_back(bestFreq);

		sortAndDeduplicate(plotFreqs, 1e-5);

		outPlotNodes->clear();
		outPlotNodes->reserve(plotFreqs.size());
		for (double pf : plotFreqs)
		{
			double g = evalGainDb(pf);
			outPlotNodes->push_back(FilterNode(pf, g));
		}
	}

	return maxPeakDb;
}
