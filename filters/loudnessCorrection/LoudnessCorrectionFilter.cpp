/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Alexander Walch

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
#include "LoudnessCorrectionFilter.h"
#include "helpers/MemoryHelper.h"

#include "VolumeController.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>

LoudnessCorrectionFilter::LoudnessCorrectionFilter(const FilterParameters& fParameters)
{
	_parameters = fParameters;
	if (_parameters.attenuation > 1.0)
	{
		_parameters.attenuation = 1.0;
	}
	if (_parameters.attenuation < 0.0)
	{
		_parameters.attenuation = 0.0;
	}
	InitializeCriticalSection(&_parameterUpdateSection);
}

LoudnessCorrectionFilter::~LoudnessCorrectionFilter()
{
	if (_stopParameterUpdateThreadEvent)
	{
		SetEvent(_stopParameterUpdateThreadEvent);
	}
	WaitForSingleObject(_parameterUpdateThreadHandle, INFINITE);
	DeleteCriticalSection(&_parameterUpdateSection);
	CloseHandle(_stopParameterUpdateThreadEvent);
	CloseHandle(_parameterUpdateThreadHandle);
	CloseHandle(_parameterchangedEvent);
}

std::vector<std::wstring> LoudnessCorrectionFilter::initialize(float sampleRate, unsigned maxFrameCount, std::vector<std::wstring> channelNames)
{
	this->_channelCount = channelNames.size();
	_lowShelfBiquads.resize(_channelCount);
	_highShelfBiquads.resize(_channelCount);
	_sampleRate = sampleRate;
	_attFactor = 1.0;
	_neutral = true;

	double freqLS = 75, qLS = 1, gainLS = 0;
	double freqHS = 10000, qHS = 1, gainHS = 0;
	VolumeController VolumeController;
	float vol;
	HRESULT res = VolumeController.getVolume(vol);
	if (res == S_OK)
	{
		double preAmp;
		getLShelfParamter(vol, freqLS, qLS, gainLS, preAmp);
		_attFactor = exp(preAmp / 6 * log(2));
		getHShelfParamter(vol + (float)preAmp, freqHS, qHS, gainHS);
		_neutral = std::max<double>(std::abs(gainLS), std::abs(gainHS)) < 0.2 ? true : false;
	}

	for (unsigned i = 0; i < _channelCount; i++)
	{
		_lowShelfBiquads[i] = BiQuad(BiQuad::LOW_SHELF, gainLS, freqLS, _sampleRate, qLS, false);
		_highShelfBiquads[i] = BiQuad(BiQuad::HIGH_SHELF, gainHS, freqHS, _sampleRate, qHS, false);
	}
	_stopParameterUpdateThreadEvent = CreateEvent(NULL, true, false, NULL);
	_parameterchangedEvent = CreateEvent(NULL, true, false, NULL);
	_parameterUpdateThreadHandle = CreateThread(NULL, 0, &parameterUpdateThread, this, 0, NULL);

	return channelNames;
}

void LoudnessCorrectionFilter::getLShelfParamter(const float& volume, double& frequence, double& q, double& gain, double& preAmp)
{
	frequence = 75;
	q = 0.52;
	float volDiff = _parameters.referenceLevel - _parameters.referenceOffset - volume;
	if (volDiff > 0)
	{
		// old: gain=volDiff*0.55*_parameters.attenuation;
		gain = volDiff * 0.55 / (1 - 0.55) * _parameters.attenuation;
		preAmp = -gain;
	}
	else if (volDiff < 0)
	{
		preAmp = 0.0;
		gain = volDiff * 0.55 * exp(volDiff / 90.0) * _parameters.attenuation;
	}
	else
	{
		gain = 0;
	}
}
void LoudnessCorrectionFilter::getHShelfParamter(const float& volume, double& frequence, double& q, double& gain)
{
	frequence = 10000;
	q = 0.9;
	float volDiff = _parameters.referenceLevel - _parameters.referenceOffset - volume;
	if (volDiff > 0)
	{
		gain = volDiff * 0.225 * exp(-volDiff / 100.0) * _parameters.attenuation;
	}
	else if (volDiff < 0)
	{
		gain = volDiff * 0.175 * exp(volDiff / 80.0) * _parameters.attenuation;
	}
	else
	{
		gain = 0;
	}
}

unsigned long __stdcall LoudnessCorrectionFilter::parameterUpdateThread(void* parameter)
{
	LoudnessCorrectionFilter* lCorrection = (LoudnessCorrectionFilter*)parameter;
	VolumeController VolumeController;
	float volOld(lCorrection->_parameters.referenceLevel);
	float vol(lCorrection->_parameters.referenceLevel);
	double freqLS, qLS, gainLS, preAmp;
	double freqHS, qHS, gainHS;
	HRESULT res;
	while (WaitForSingleObject(lCorrection->_stopParameterUpdateThreadEvent, 0) == WAIT_TIMEOUT)
	{
		if (WaitForSingleObject(lCorrection->_parameterchangedEvent, 0) == WAIT_TIMEOUT)
		{
			res = VolumeController.getVolume(vol);
			if (res == S_OK)
			{
				if (vol != volOld)
				{
					lCorrection->getLShelfParamter(vol, freqLS, qLS, gainLS, preAmp);
					lCorrection->_attFactor = exp(preAmp / 6 * log(2));
					lCorrection->getHShelfParamter(vol + (float)preAmp, freqHS, qHS, gainHS);
					lCorrection->upDateBiquadCoefficients(freqHS, qHS, gainHS, true);
					lCorrection->upDateBiquadCoefficients(freqLS, qLS, gainLS, false);
					volOld = vol;

					lCorrection->_neutralUpDate = std::max<double>(std::abs(gainLS), std::abs(gainHS)) < 0.2 ? true : false;

					SetEvent(lCorrection->_parameterchangedEvent);
				}
			}
		}
		Sleep(10);
		// ==========================
	}
	return 0;
}

bool LoudnessCorrectionFilter::upDateNeutral()
{
	return _neutralUpDate;
}

#pragma AVRT_CODE_BEGIN
void LoudnessCorrectionFilter::process(float** output, float** input, unsigned frameCount)
{
	if (_parameters.state == false)
	{
		for (unsigned int j = 0; j < frameCount; j++)
		{
			for (unsigned i = 0; i < _channelCount; i++)
			{
				output[i][j] = input[i][j];
			}
		}
		output = input;
		return;
	}
	if (WaitForSingleObject(_parameterchangedEvent, 0) == WAIT_OBJECT_0)
	{
		for (unsigned i = 0; i < _channelCount; i++)
		{
			_lowShelfBiquads[i].setCoefficients(_aLS, _a0LS);
			_highShelfBiquads[i].setCoefficients(_aHS, _a0HS);
		}
		_neutral = upDateNeutral();
		ResetEvent(_parameterchangedEvent);
	}
	for (unsigned i = 0; i < _channelCount; i++)
	{
		float* inputChannel = input[i];
		float* outputChannel = output[i];
		for (unsigned j = 0; j < frameCount; j++)
		{
			_tempResult = _lowShelfBiquads[i].process(inputChannel[j]);
			_lowShelfBiquads[i].removeDenormals();
			_tempResult *= _attFactor;
			outputChannel[j] = (float)_highShelfBiquads[i].process(_tempResult);
			_highShelfBiquads[i].removeDenormals();

			// if there is nearly no loudness correction necessary => set output=input to achive best quality
			if (_neutral)
			{
				outputChannel[j] = inputChannel[j];
			}
		}
	}
}

void LoudnessCorrectionFilter::upDateBiquadCoefficients(const double& freq, const double& bandwidthOrQOrS, const double& dbGain, bool highshelf)
{
	double A;
	A = pow(10, dbGain / 40);

	double omega = 2 * M_PI * freq / _sampleRate;
	double sn = sin(omega);
	double cs = cos(omega);
	double alpha;
	double beta = -1;

	alpha = sn / 2 * sqrt((A + 1 / A) * (1 / bandwidthOrQOrS - 1) + 2);
	beta = 2 * sqrt(A) * alpha;

	double a0;
	TryEnterCriticalSection(&_parameterUpdateSection);
	if (highshelf)
	{
		a0 = (A + 1) - (A - 1) * cs + beta;
		_a0HS = (float)((A * ((A + 1) + (A - 1) * cs + beta)) / a0);
		_aHS[0] = (float)((-2 * A * ((A - 1) + (A + 1) * cs)) / a0);
		_aHS[1] = (float)((A * ((A + 1) + (A - 1) * cs - beta)) / a0);

		_aHS[2] = (float)((2 * ((A - 1) - (A + 1) * cs)) / a0);
		_aHS[3] = (float)(((A + 1) - (A - 1) * cs - beta) / a0);
	}
	else
	{
		a0 = (A + 1) + (A - 1) * cs + beta;
		_a0LS = (float)((A * ((A + 1) - (A - 1) * cs + beta)) / a0);
		_aLS[0] = (float)((2 * A * ((A - 1) - (A + 1) * cs)) / a0);
		_aLS[1] = (float)((A * ((A + 1) - (A - 1) * cs - beta)) / a0);

		_aLS[2] = (float)((-2 * ((A - 1) + (A + 1) * cs)) / a0);
		_aLS[3] = (float)(((A + 1) + (A - 1) * cs - beta) / a0);
	}
	LeaveCriticalSection(&_parameterUpdateSection);
}
#pragma AVRT_CODE_END
