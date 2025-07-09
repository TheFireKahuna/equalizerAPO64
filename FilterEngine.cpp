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
#include <sstream>
#include <fstream>
#include <algorithm>
#include <exception>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlwapi.h>
#include <Ks.h>
#include <KsMedia.h>
#include <mpParser.h>
#include <mpPackageCommon.h>
#include <mpPackageNonCmplx.h>
#include <mpPackageStr.h>
#include <mpPackageMatrix.h>

#include "helpers/RegistryHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/MemoryHelper.h"
#include "helpers/ChannelHelper.h"
#include "FilterEngine.h"
#include "filters/ExpressionFilterFactory.h"
#include "filters/DeviceFilterFactory.h"
#include "filters/StageFilterFactory.h"
#include "filters/IfFilterFactory.h"
#include "filters/ChannelFilterFactory.h"
#include "filters/BiQuadFilterFactory.h"
#include "filters/IIRFilterFactory.h"
#include "filters/PreampFilterFactory.h"
#include "filters/DelayFilterFactory.h"
#include "filters/CopyFilterFactory.h"
#include "filters/IncludeFilterFactory.h"
#include "filters/ConvolutionFilterFactory.h"
#include "filters/GraphicEQFilterFactory.h"
#include "filters/VSTPluginFilterFactory.h"
#include "filters/loudnessCorrection/LoudnessCorrectionFilterFactory.h"

using namespace std;
using namespace mup;

FilterEngine::FilterEngine()
	: parser(nullptr),
      allocatedFrameCount(0),
	  preMix(false),
	  capture(false),
	  postMixInstalled(true),
	  inputChannelCount(0),
      realChannelCount(0),
      outputChannelCount(0),
	  lastInputWasSilent(false),
	  threadHandle(nullptr),
	  currentConfig(nullptr),
	  nextConfig(nullptr),
	  previousConfig(nullptr),
	  transitionCounter(0)
{
	InitializeCriticalSection(&loadSection);
	loadSemaphore = CreateSemaphore(NULL, 1, 1, NULL);
	parser = new ParserX();
	parser->EnableAutoCreateVar(true);

	factories.push_back(new DeviceFilterFactory());
	factories.push_back(new IfFilterFactory());
	factories.push_back(new ExpressionFilterFactory());
	factories.push_back(new IncludeFilterFactory());
	factories.push_back(new StageFilterFactory());
	factories.push_back(new ChannelFilterFactory());
	factories.push_back(new IIRFilterFactory());
	factories.push_back(new BiQuadFilterFactory());
	factories.push_back(new PreampFilterFactory());
	factories.push_back(new DelayFilterFactory());
	factories.push_back(new CopyFilterFactory());
	factories.push_back(new ConvolutionFilterFactory());
	factories.push_back(new GraphicEQFilterFactory());
	factories.push_back(new VSTPluginFilterFactory());
	factories.push_back(new LoudnessCorrectionFilterFactory());
}

FilterEngine::~FilterEngine()
{
	// Make sure notification thread is terminated before cleaning up, otherwise deleted memory might be accessed in loadConfig
	if (threadHandle != NULL)
	{
		SetEvent(shutdownEvent);
		if (WaitForSingleObject(threadHandle, INFINITE) == WAIT_OBJECT_0)
		{
			TraceF(L"Successfully terminated directory change notification thread");
		}
		CloseHandle(shutdownEvent);
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}

	cleanupConfigurations();

	for (IFilterFactory* factory : factories)
		delete factory;

	delete parser;
	CloseHandle(loadSemaphore);
	DeleteCriticalSection(&loadSection);
}

void FilterEngine::resizeBuffers(unsigned frameCount) {
	if (allocatedFrameCount < frameCount || inputBuf2D.size() != realChannelCount || outputBuf2D.size() != outputChannelCount) {

		TraceF(L"Reallocating internal double-precision buffers for %u frames and %u/%u channels.", frameCount, realChannelCount, outputChannelCount);
		allocatedFrameCount = frameCount;

		// Resize 1D buffers (for interleaved audio)
		try {
			inputBuf1D.resize(realChannelCount * frameCount);
			outputBuf1D.resize(outputChannelCount * frameCount);

			// Resize 2D buffers (for non-interleaved audio)
			inputBuf2D.resize(realChannelCount);
			for (unsigned i = 0; i < realChannelCount; ++i) {
				inputBuf2D[i] = make_unique<double[]>(frameCount);
			}
			outputBuf2D.resize(outputChannelCount);
			for (unsigned i = 0; i < outputChannelCount; ++i) {
				outputBuf2D[i] = make_unique<double[]>(frameCount);
			}
		}
		catch (const std::bad_alloc& e) {
			LogF(L"FATAL: Failed to allocate audio buffers. Exception: %S", e.what());
			allocatedFrameCount = 0;
		}
	}
}

void FilterEngine::setPreMix(bool preMix)
{
	this->preMix = preMix;
}

void FilterEngine::setDeviceInfo(bool capture, bool postMixInstalled, const wstring& deviceName, const wstring& connectionName, const wstring& deviceGuid, const wstring& deviceString)
{
	this->capture = capture;
	this->postMixInstalled = postMixInstalled;
	this->deviceName = deviceName;
	this->connectionName = connectionName;
	this->deviceGuid = deviceGuid;
	this->deviceString = deviceString;
}

void FilterEngine::initialize(float sampleRate, unsigned inputChannelCount, unsigned realChannelCount, unsigned outputChannelCount, unsigned channelMask, unsigned maxFrameCount, const wstring& customPath)
{
	EnterCriticalSection(&loadSection);

	cleanupConfigurations();

	this->sampleRate = sampleRate;
	this->inputChannelCount = inputChannelCount;
	this->realChannelCount = realChannelCount;
	this->outputChannelCount = outputChannelCount;
	this->maxFrameCount = maxFrameCount;
	this->transitionCounter = 0;
	this->transitionLength = (unsigned)(sampleRate / 100);
	resizeBuffers(maxFrameCount);

	unsigned deviceChannelCount;
	if (capture)
		deviceChannelCount = inputChannelCount;
	else
		deviceChannelCount = outputChannelCount;

	if (channelMask == 0)
		channelMask = ChannelHelper::getDefaultChannelMask(deviceChannelCount);

	this->channelMask = channelMask;

	vector<wstring> channelNames = ChannelHelper::getChannelNames(deviceChannelCount, channelMask);
	TraceF(L"%d channels for this device: %s", deviceChannelCount, StringHelper::join(channelNames, L" ").c_str());

	try
	{
		configPath = RegistryHelper::readValue(APP_REGPATH, L"ConfigPath");
	}
	catch (RegistryException e)
	{
		LogF(L"Can't read config path because of: %s", e.getMessage().c_str());
		LeaveCriticalSection(&loadSection);
		return;
	}

	parser->ClearConst();
	parser->ClearFun();
	parser->ClearInfixOprt();
	parser->ClearOprt();
	parser->ClearPostfixOprt();
	parser->AddPackage(PackageCommon::Instance());
	parser->AddPackage(PackageNonCmplx::Instance());
	parser->AddPackage(PackageStr::Instance());
	parser->AddPackage(PackageMatrix::Instance());

	for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
	{
		IFilterFactory* factory = *it;
		factory->initialize(this);
	}

	if (configPath != L"")
	{
		loadConfig(customPath);

		if (threadHandle == NULL && customPath.empty())
		{
			shutdownEvent = CreateEventW(NULL, true, false, NULL);
			threadHandle = CreateThread(NULL, 0, notificationThread, this, 0, NULL);
			if (threadHandle == INVALID_HANDLE_VALUE)
				threadHandle = NULL;
			else
				TraceF(L"Successfully created directory change notification thread %d for %s and its subtree", GetThreadId(threadHandle), configPath.c_str());
		}
	}
	LeaveCriticalSection(&loadSection);
}

void FilterEngine::loadConfig(const wstring& customPath)
{
	EnterCriticalSection(&loadSection);
	timer.start();
	if (previousConfig != NULL)
	{
		previousConfig->~FilterConfiguration();
		MemoryHelper::free(previousConfig);
		previousConfig = NULL;
	}

	allChannelNames = ChannelHelper::getChannelNames(max(realChannelCount, outputChannelCount), channelMask);

	currentChannelNames = allChannelNames;
	lastChannelNames.clear();
	lastNewChannelNames.clear();
	watchRegistryKeys.clear();
	parser->ClearVar();

	for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
	{
		IFilterFactory* factory = *it;
		vector<IFilter*> newFilters = factory->startOfConfiguration();
		if (!newFilters.empty())
			addFilters(newFilters);
	}

	if (customPath.empty())
		loadConfigFile(configPath + L"\\config.txt");
	else
		loadConfigFile(customPath);

	for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
	{
		IFilterFactory* factory = *it;
		vector<IFilter*> newFilters = factory->endOfConfiguration();
		if (!newFilters.empty())
			addFilters(newFilters);
	}

	void* mem = MemoryHelper::alloc(sizeof(FilterConfiguration));
	FilterConfiguration* config = new(mem) FilterConfiguration(this, filterInfos, (unsigned)allChannelNames.size());

	filterInfos.clear();

	double loadTime = timer.stop();
	TraceF(L"Finished loading configuration after %lf milliseconds", loadTime * 1000.0);

	if (currentConfig == NULL)
		currentConfig = config;
	else
		nextConfig = config;

	LeaveCriticalSection(&loadSection);
}

void FilterEngine::loadConfigFile(const wstring& path)
{
	TraceF(L"Loading configuration from %s", path.c_str());

	HANDLE hFile = INVALID_HANDLE_VALUE;
	while (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			DWORD error = GetLastError();
			if (error != ERROR_SHARING_VIOLATION)
			{
				LogF(L"Error while reading configuration file %s: %s", path.c_str(), StringHelper::getSystemErrorString(error).c_str());
				return;
			}

			// file is being written, so wait
			Sleep(1);
		}
	}

	stringstream inputStream;

	char buf[8192];
	unsigned long bytesRead = -1;
	while (ReadFile(hFile, buf, sizeof(buf), &bytesRead, NULL) && bytesRead != 0)
	{
		inputStream.write(buf, bytesRead);
	}

	CloseHandle(hFile);

	inputStream.seekg(0);

	vector<wstring> savedChannelNames = currentChannelNames;

	for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
	{
		IFilterFactory* factory = *it;
		vector<IFilter*> newFilters = factory->startOfFile(path);
		if (!newFilters.empty())
			addFilters(newFilters);
	}

	while (inputStream.good())
	{
		string encodedLine;
		getline(inputStream, encodedLine);
		if (encodedLine.size() > 0 && encodedLine[encodedLine.size() - 1] == '\r')
			encodedLine.resize(encodedLine.size() - 1);

		wstring line = StringHelper::toWString(encodedLine, CP_UTF8);
		if (line.find(L'\uFFFD') != -1)
			line = StringHelper::toWString(encodedLine, CP_ACP);

		size_t pos = line.find(L':');
		if (pos != -1)
		{
			wstring key = line.substr(0, pos);
			wstring value = line.substr(pos + 1);

			// allow to use indentation
			key = StringHelper::trim(key);

			for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
			{
				IFilterFactory* factory = *it;

				vector<IFilter*> newFilters;
				try
				{
					newFilters = factory->createFilter(path, key, value);
				}
				catch (exception e)
				{
					LogF(L"%S", e.what());
				}

				if (key == L"")
					break;
				if (!newFilters.empty())
				{
					addFilters(newFilters);
					break;
				}
			}
		}
	}

	for (vector<IFilterFactory*>::const_iterator it = factories.cbegin(); it != factories.cend(); it++)
	{
		IFilterFactory* factory = *it;
		vector<IFilter*> newFilters = factory->endOfFile(path);
		if (!newFilters.empty())
			addFilters(newFilters);
	}

	// restore channels selected in outer configuration file
	currentChannelNames = savedChannelNames;
}

void FilterEngine::watchRegistryKey(const std::wstring& key)
{
	watchRegistryKeys.insert(key);
}

#pragma AVRT_CODE_BEGIN
void convertFloatToDouble(double* dest, const float* src, size_t count) {
#if defined(__AVX512F__) && !defined(_M_ARM64) // AVX-512 Path (e.g., Zen 4, some Intel CPUs)
	size_t i = 0;
	for (; i + 16 <= count; i += 16) {
		// Load 16 floats
		__m512 float_vec = _mm512_loadu_ps(src + i);
		// Convert the lower 8 floats to 8 doubles
		__m512d double_vec_lo = _mm512_cvtps_pd(_mm512_extractf32x8_ps(float_vec, 0));
		// Convert the upper 8 floats to 8 doubles
		__m512d double_vec_hi = _mm512_cvtps_pd(_mm512_extractf32x8_ps(float_vec, 1));
		// Store the 16 resulting doubles
		_mm512_storeu_pd(dest + i, double_vec_lo);
		_mm512_storeu_pd(dest + i + 8, double_vec_hi);
	}
	// Handle any remaining elements
	for (; i < count; ++i) dest[i] = static_cast<double>(src[i]);
#elif defined(__AVX2__) && !defined(_M_ARM64) // AVX2 / AVX Fallback Path (e.g., Zen 2/3)
	size_t i = 0;
	for (; i + 8 <= count; i += 8) {
		// Load 8 floats into a 256-bit register
		__m256 float_vec = _mm256_loadu_ps(src + i);
		// Convert the lower 4 floats to 4 doubles
		__m256d double_vec_lo = _mm256_cvtps_pd(_mm256_extractf128_ps(float_vec, 0));
		// Convert the upper 4 floats to 4 doubles
		__m256d double_vec_hi = _mm256_cvtps_pd(_mm256_extractf128_ps(float_vec, 1));
		// Store the 8 resulting doubles
		_mm256_storeu_pd(dest + i, double_vec_lo);
		_mm256_storeu_pd(dest + i + 4, double_vec_hi);
	}
	// Handle any remaining elements
	for (; i < count; ++i) dest[i] = static_cast<double>(src[i]);
#else // Scalar fallback for non-x86 or very old CPUs
	for (size_t i = 0; i < count; ++i) dest[i] = static_cast<double>(src[i]);
#endif
}

// Converts a block of doubles back to floats.
void convertDoubleToFloat(float* dest, const double* src, size_t count) {
#if defined(__AVX512F__) && !defined(_M_ARM64) // AVX-512 Path
	size_t i = 0;
	for (; i + 16 <= count; i += 16) {
		// Load 16 doubles from memory
		__m512d double_vec_lo = _mm512_loadu_pd(src + i);
		__m512d double_vec_hi = _mm512_loadu_pd(src + i + 8);
		// Convert 8 doubles to 8 floats
		__m256 float_vec_lo = _mm512_cvtpd_ps(double_vec_lo);
		// Convert another 8 doubles to 8 floats
		__m256 float_vec_hi = _mm512_cvtpd_ps(double_vec_hi);
		// Combine the two 256-bit float vectors into one 512-bit vector
		__m512 float_vec = _mm512_insertf32x8(_mm512_castps256_ps512(float_vec_lo), float_vec_hi, 1);
		_mm512_storeu_ps(dest + i, float_vec);
	}
	for (; i < count; ++i) dest[i] = static_cast<float>(src[i]);
#elif defined(__AVX2__) && !defined(_M_ARM64) // AVX2 / AVX Fallback Path
	size_t i = 0;
	for (; i + 8 <= count; i += 8) {
		// Load 8 doubles from memory
		__m256d double_vec_lo = _mm256_loadu_pd(src + i);
		__m256d double_vec_hi = _mm256_loadu_pd(src + i + 4);
		// Convert 4 doubles to 4 floats
		__m128 float_vec_lo = _mm256_cvtpd_ps(double_vec_lo);
		// Convert another 4 doubles to 4 floats
		__m128 float_vec_hi = _mm256_cvtpd_ps(double_vec_hi);
		// Combine the two 128-bit float vectors into one 256-bit vector
		__m256 float_vec = _mm256_insertf128_ps(_mm256_castps128_ps256(float_vec_lo), float_vec_hi, 1);
		_mm256_storeu_ps(dest + i, float_vec);
	}
	for (; i < count; ++i) dest[i] = static_cast<float>(src[i]);
#else // Scalar fallback
	for (size_t i = 0; i < count; ++i) dest[i] = static_cast<float>(src[i]);
#endif
}


// Process interleaved audio (float*)
void FilterEngine::process(float* output, float* input, unsigned frameCount)
{
	if (currentConfig->isEmpty() && nextConfig == NULL)
	{
		// Bypass mode: if no filters are active, just copy input to output if necessary.
		if (realChannelCount == outputChannelCount && input != output) {
			memcpy(output, input, outputChannelCount * frameCount * sizeof(float));
		}
		return;
	}

	// Ensure our internal buffers are large enough
	resizeBuffers(frameCount);

	// Conversion from float to double using SIMD
	const unsigned inputSampleCount = realChannelCount * frameCount;
	convertFloatToDouble(inputBuf1D.data(), input, inputSampleCount);

	// The core processing logic remains unchanged
	currentConfig->read(inputBuf1D.data(), frameCount);
	currentConfig->process(frameCount);

	if (nextConfig != NULL)
	{
		nextConfig->read(inputBuf1D.data(), frameCount);
		nextConfig->process(frameCount);
		transitionCounter = currentConfig->doTransition(nextConfig, frameCount, transitionCounter, transitionLength);
	}

	currentConfig->write(outputBuf1D.data(), frameCount);

	// Conversion from double back to float using SIMD
	const unsigned outputSampleCount = outputChannelCount * frameCount;
	convertDoubleToFloat(output, outputBuf1D.data(), outputSampleCount);

	if (nextConfig != NULL && transitionCounter >= transitionLength)
	{
		previousConfig = currentConfig;
		currentConfig = nextConfig;
		nextConfig = NULL;
		transitionCounter = 0;
		ReleaseSemaphore(loadSemaphore, 1, NULL);
	}
}

// Process non-interleaved audio (float**)
void FilterEngine::process(float** output, float** input, unsigned frameCount)
{
	if (currentConfig->isEmpty() && nextConfig == NULL)
	{
		// Bypass mode
		if (realChannelCount == outputChannelCount && input != output) {
			for (unsigned c = 0; c < realChannelCount; c++)
				memcpy(output[c], input[c], frameCount * sizeof(float));
		}
		return;
	}

	resizeBuffers(frameCount);

	// Create temporary raw pointer arrays for the FilterConfiguration interface
	vector<double*> tempInputPtrs(realChannelCount);
	vector<double*> tempOutputPtrs(outputChannelCount);
	for (unsigned i = 0; i < realChannelCount; ++i) tempInputPtrs[i] = inputBuf2D[i].get();
	for (unsigned i = 0; i < outputChannelCount; ++i) tempOutputPtrs[i] = outputBuf2D[i].get();

	// Optimized conversion for each channel
	for (unsigned c = 0; c < realChannelCount; c++) {
		convertFloatToDouble(inputBuf2D[c].get(), input[c], frameCount);
	}

	// Core processing logic is the same
	currentConfig->read(tempInputPtrs.data(), frameCount);
	currentConfig->process(frameCount);

	if (nextConfig != NULL)
	{
		nextConfig->read(tempInputPtrs.data(), frameCount);
		nextConfig->process(frameCount);
		transitionCounter = currentConfig->doTransition(nextConfig, frameCount, transitionCounter, transitionLength);
	}

	currentConfig->write(tempOutputPtrs.data(), frameCount);

	// Optimized conversion back for each channel
	for (unsigned c = 0; c < outputChannelCount; c++) {
		convertDoubleToFloat(output[c], outputBuf2D[c].get(), frameCount);
	}

	// Transition logic remains the same
	if (nextConfig != NULL && transitionCounter >= transitionLength)
	{
		previousConfig = currentConfig;
		currentConfig = nextConfig;
		nextConfig = NULL;
		transitionCounter = 0;
		ReleaseSemaphore(loadSemaphore, 1, NULL);
	}
}
#pragma AVRT_CODE_END

void FilterEngine::addFilters(vector<IFilter*> filters)
{
	for (vector<IFilter*>::iterator it = filters.begin(); it != filters.end(); it++)
	{
		IFilter* filter = *it;
		FilterInfo* filterInfo = (FilterInfo*)MemoryHelper::alloc(sizeof(FilterInfo));
		filterInfo->filter = filter;
		filterInfo->inPlace = filter->getInPlace();
		vector<wstring> savedChannelNames = currentChannelNames;
		bool allChannels = filter->getAllChannels();
		if (allChannels)
			currentChannelNames = allChannelNames;

		if (lastChannelNames == currentChannelNames)
		{
			filterInfo->inChannelCount = 0;
			filterInfo->inChannels = NULL;
		}
		else
		{
			filterInfo->inChannelCount = currentChannelNames.size();
			filterInfo->inChannels = (size_t*)MemoryHelper::alloc(filterInfo->inChannelCount * sizeof(size_t));

			size_t c = 0;
			for (vector<wstring>::iterator it2 = currentChannelNames.begin(); it2 != currentChannelNames.end(); it2++)
			{
				vector<wstring>::iterator pos = find(allChannelNames.begin(), allChannelNames.end(), *it2);
				filterInfo->inChannels[c++] = pos - allChannelNames.begin();
			}
		}

		lastChannelNames = currentChannelNames;

		vector<wstring> newChannelNames = filter->initialize(sampleRate, maxFrameCount, currentChannelNames);

		if (filterInfo->inPlace && lastInPlace && lastNewChannelNames == newChannelNames)
		{
			filterInfo->outChannelCount = 0;
			filterInfo->outChannels = NULL;
		}
		else
		{
			filterInfo->outChannelCount = newChannelNames.size();
			filterInfo->outChannels = (size_t*)MemoryHelper::alloc(filterInfo->outChannelCount * sizeof(size_t));

			size_t c = 0;
			for (vector<wstring>::iterator it2 = newChannelNames.begin(); it2 != newChannelNames.end(); it2++)
			{
				vector<wstring>::iterator pos = find(allChannelNames.begin(), allChannelNames.end(), *it2);
				if (pos == allChannelNames.end())
				{
					filterInfo->outChannels[c++] = allChannelNames.size();
					allChannelNames.push_back(*it2);
				}
				else
				{
					filterInfo->outChannels[c++] = pos - allChannelNames.begin();
				}
			}
		}

		lastNewChannelNames = newChannelNames;
		lastInPlace = filterInfo->inPlace;
		if (!lastInPlace)
			swap(lastChannelNames, lastNewChannelNames);

		filterInfos.push_back(filterInfo);

		if (filter->getSelectChannels())
			currentChannelNames = newChannelNames;
		else
			currentChannelNames = savedChannelNames;
	}
}

void FilterEngine::cleanupConfigurations()
{
	if (currentConfig != NULL)
	{
		currentConfig->~FilterConfiguration();
		MemoryHelper::free(currentConfig);
		currentConfig = NULL;
	}

	if (nextConfig != NULL)
	{
		nextConfig->~FilterConfiguration();
		MemoryHelper::free(nextConfig);
		nextConfig = NULL;
	}

	if (previousConfig != NULL)
	{
		previousConfig->~FilterConfiguration();
		MemoryHelper::free(previousConfig);
		previousConfig = NULL;
	}
}

unsigned long __stdcall FilterEngine::notificationThread(void* parameter)
{
	FilterEngine* engine = (FilterEngine*)parameter;

	HANDLE notificationHandle = FindFirstChangeNotificationW(engine->configPath.c_str(), true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
	if (notificationHandle == INVALID_HANDLE_VALUE)
		notificationHandle = NULL;

	HANDLE registryEvent = CreateEventW(NULL, true, false, NULL);

	HANDLE handles[3] = {engine->shutdownEvent, notificationHandle, registryEvent};
	while (true)
	{
		vector<HKEY> keyHandles;
		for (auto it = engine->watchRegistryKeys.begin(); it != engine->watchRegistryKeys.end(); it++)
		{
			try
			{
				HKEY keyHandle = RegistryHelper::openKey(*it, KEY_NOTIFY | KEY_WOW64_64KEY);
				keyHandles.push_back(keyHandle);
				RegNotifyChangeKeyValue(keyHandle, false, REG_NOTIFY_CHANGE_LAST_SET, registryEvent, true);
			}
			catch (RegistryException e)
			{
				LogFStatic(L"%s", e.getMessage().c_str());
			}
		}

		DWORD which = WaitForMultipleObjects(3, handles, false, INFINITE);

		for (auto it = keyHandles.begin(); it != keyHandles.end(); it++)
		{
			RegCloseKey(*it);
		}

		if (which == WAIT_OBJECT_0)
		{
			// Shutdown
			break;
		}
		else
		{
			if (which == WAIT_OBJECT_0 + 1)
			{
				FindNextChangeNotification(notificationHandle);
				// Wait for second event within 10 milliseconds to avoid loading twice
				WaitForMultipleObjects(1, &notificationHandle, false, 10);
			}

			HANDLE handles[2] = {engine->shutdownEvent, engine->loadSemaphore};
			DWORD which = WaitForMultipleObjects(2, handles, false, INFINITE);
			if (which == WAIT_OBJECT_0)
			{
				// Shutdown
				break;
			}

			engine->loadConfig();
			FindNextChangeNotification(notificationHandle);
			ResetEvent(registryEvent);
		}
	}

	FindCloseChangeNotification(notificationHandle);
	CloseHandle(registryEvent);

	return 0;
}
