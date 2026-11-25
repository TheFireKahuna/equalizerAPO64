/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

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

#include <algorithm>
#include <QElapsedTimer>

#include "FilterEngine.h"
#include "AnalysisThread.h"

using namespace std;

AnalysisThread::AnalysisThread()
{
}

AnalysisThread::~AnalysisThread()
{
	mutex.lock();
	quit = true;
	condition.wakeAll();
	mutex.unlock();

	wait();

	if (resultFreqData != NULL)
		fftw_free(resultFreqData);

	if (buf != NULL)
		delete[] buf;
	if (buf2 != NULL)
		delete[] buf2;
	if (timeData != NULL)
		fftw_free(timeData);
	if (freqData != NULL)
		fftw_free(freqData);
	if (planForward != NULL)
		fftw_destroy_plan(planForward);
}

void AnalysisThread::setParameters(shared_ptr<AbstractAPOInfo> device, int channelMask, int channelIndex, QString configPath, int frameCount)
{
	QMutexLocker mutexLocker(&mutex);
	this->device = device;
	this->channelMask = channelMask;
	this->channelIndex = channelIndex;
	this->configPath = configPath;
	this->frameCount = std::max(128, frameCount);

	condition.wakeAll();
}

void AnalysisThread::beginGetResult()
{
	mutex.lock();
}

void AnalysisThread::endGetResult()
{
	mutex.unlock();
}

fftw_complex* AnalysisThread::getFreqData() const
{
	return resultFreqData;
}

int AnalysisThread::getFreqDataLength() const
{
	return freqDataLength;
}

int AnalysisThread::getFreqDataSampleRate() const
{
	return freqDataSampleRate;
}

const vector<FilterNode>& AnalysisThread::getPlotNodes() const
{
	return plotNodes;
}

double AnalysisThread::getPeakGain() const
{
	return peakGain;
}

bool AnalysisThread::isSafetyBypassed() const
{
	return safetyBypassed;
}

int AnalysisThread::getInvalidLineCount() const
{
	return invalidLineCount;
}

int AnalysisThread::getOverflowLineCount() const
{
	return overflowLineCount;
}

int AnalysisThread::getLatency() const
{
	return latency;
}

double AnalysisThread::getInitializationTime() const
{
	return initializationTime;
}

double AnalysisThread::getProcessingTime() const
{
	return processingTime;
}

unsigned AnalysisThread::getProcessedFrames() const
{
	return processedFrames;
}

void AnalysisThread::run()
{
	while (true)
	{
		mutex.lock();
		if (!quit && this->frameCount == 0)
			condition.wait(&mutex);
		if (quit)
		{
			mutex.unlock();
			break;
		}

		shared_ptr<AbstractAPOInfo> device = this->device;
		int channelMask = this->channelMask;
		int channelIndex = this->channelIndex;
		QString configPath = this->configPath;
		int frameCount = this->frameCount;
		this->frameCount = 0;
		mutex.unlock();

		QElapsedTimer timer;
		timer.start();

		unsigned channelCount = (device != NULL) ? device->getChannelCount() : 2;
		if (device != NULL && channelMask != 0 && channelMask != (int)device->getChannelMask())
		{
			channelCount = 0;
			for (int i = 0; i < 31; i++)
			{
				int channelPos = 1 << i;
				if (channelMask & channelPos)
					channelCount++;
			}
		}
		if (channelCount == 0)
		{
			channelCount = 2;
			channelMask = KSAUDIO_SPEAKER_STEREO;
		}

		unsigned sampleRate = (device != NULL) ? device->getSampleRate() : 48000;
		if (sampleRate == 0)
			sampleRate = 48000;

		int safeChannelIndex = std::clamp(channelIndex, 0, (int)channelCount - 1);

		qint64 startTime = timer.nsecsElapsed();

		FilterEngine engine;
		engine.setAnalysisMode(true);
		if (device != NULL)
			engine.setDeviceInfo(device->isInput(), true, device->getDeviceName(), device->getConnectionName(), device->getDeviceGuid(), device->getDeviceString());
		else
			engine.setDeviceInfo(false, true, L"Default", L"Default", L"", L"");
		engine.initialize(sampleRate, channelCount, channelCount, channelCount, channelMask, frameCount, configPath.toStdWString());
		double initializationTime = (timer.nsecsElapsed() - startTime) / 1e6;

		if (frameCount != lastFrameCount || (int)channelCount != lastChannelCount)
		{
			if (buf != NULL)
				delete[] buf;
			buf = new double[frameCount * channelCount];
			memset(buf, 0, frameCount * channelCount * sizeof(double));

			if (buf2 != NULL)
				delete[] buf2;
			buf2 = new double[frameCount * channelCount];
		}
		for (unsigned i = 0; i < channelCount; i++)
			buf[i] = 1.0;

		if (frameCount != lastFrameCount)
		{
			if (timeData != NULL)
				fftw_free(timeData);
			timeData = fftw_alloc_real(frameCount);

			if (freqData != NULL)
				fftw_free(freqData);
			freqData = fftw_alloc_complex(frameCount);

			if (planForward != NULL)
				fftw_destroy_plan(planForward);
			fftw_make_planner_thread_safe();
			planForward = fftw_plan_dft_r2c_1d(frameCount, timeData, freqData, FFTW_ESTIMATE);
		}

		lastFrameCount = frameCount;
		lastChannelCount = channelCount;

		int latency = 0;
		int startFrame = -1;
		double processingTime = 0.0;
		unsigned processedFrames = 0;
		// stop searching for startFrame after 10 seconds of audio data
		while (processedFrames < 10 * sampleRate)
		{
			qint64 procStart = timer.nsecsElapsed();
			engine.process(buf2, buf, frameCount);
			processingTime += (timer.nsecsElapsed() - procStart) / 1e6;
			processedFrames += frameCount;

			if (startFrame != -1)
			{
				for (int i = 0; i < startFrame; i++)
				{
					timeData[frameCount - startFrame + i] = buf2[i * channelCount + safeChannelIndex];
				}
				break;
			}

			for (int i = 0; i < frameCount; i++)
			{
				double s = buf2[i * channelCount + safeChannelIndex];
				if (abs(s) > 1e-5f)
				{
					startFrame = i;
					break;
				}
			}

			if (startFrame != -1)
			{
				for (int i = 0; i < frameCount - startFrame; i++)
				{
					timeData[i] = buf2[(startFrame + i) * channelCount + safeChannelIndex];
				}

				if (startFrame == 0)
					break;
			}

			if (latency == 0)
			{
				for (unsigned i = 0; i < channelCount; i++)
					buf[i] = 0.0;
			}

			if (startFrame == -1)
				latency += frameCount;
		}

		if (startFrame != -1)
		{
			latency += startFrame;
			fftw_execute(planForward);
		}
		else
		{
			latency = 0;
			memset(freqData, 0, frameCount * sizeof(fftw_complex));
		}

		// Use the exact resolution-independent peak gain and analytical curve from GainAnalysisHelper
		double exactPeakGain = engine.getExactPeakGainDb();
		bool bypassed = engine.isSafetyBypassed();
		int invalidLines = engine.getInvalidLineCount();
		int overflowLines = engine.getOverflowLineCount();
		vector<FilterNode> exactPlotNodes = engine.getAnalyticalPlotNodes();

		mutex.lock();
		if (this->freqDataLength != frameCount)
		{
			if (resultFreqData != NULL)
				fftw_free(resultFreqData);
			resultFreqData = fftw_alloc_complex(frameCount);
		}
		memcpy(resultFreqData, freqData, frameCount * sizeof(fftw_complex));
		this->freqDataLength = frameCount;
		this->freqDataSampleRate = sampleRate;
		this->plotNodes = std::move(exactPlotNodes);
		this->latency = latency;
		this->peakGain = exactPeakGain;
		this->safetyBypassed = bypassed;
		this->invalidLineCount = invalidLines;
		this->overflowLineCount = overflowLines;
		this->initializationTime = initializationTime;
		this->processingTime = processingTime;
		this->processedFrames = processedFrames;
		mutex.unlock();

		qDebug("Analysis took %.1f ms (exact peak: %.2f dB, bypassed: %d)", timer.nsecsElapsed() / 1e6, exactPeakGain, bypassed ? 1 : 0);

		emit analysisFinished();
	}
}
