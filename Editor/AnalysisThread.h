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

#pragma once

#include <vector>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <fftw3.h>

#include "DeviceAPOInfo.h"
#include "helpers/GainIterator.h"

class AnalysisThread : public QThread
{
	Q_OBJECT

public:
	AnalysisThread();
	~AnalysisThread();
	void setParameters(std::shared_ptr<AbstractAPOInfo> device, int channelMask, int channelIndex, QString configPath, int frameCount);
	void beginGetResult();
	void endGetResult();

	fftw_complex* getFreqData() const;
	int getFreqDataLength() const;
	int getFreqDataSampleRate() const;
	const std::vector<FilterNode>& getPlotNodes() const;
	double getPeakGain() const;
	bool isSafetyBypassed() const;
	int getInvalidLineCount() const;
	int getOverflowLineCount() const;
	int getLatency() const;
	double getInitializationTime() const;
	double getProcessingTime() const;
	unsigned getProcessedFrames() const;

signals:
	void analysisFinished();

protected:
	void run() override;

private:
	QMutex mutex;
	QWaitCondition condition;
	bool quit = false;

	// input
	std::shared_ptr<AbstractAPOInfo> device;
	int channelMask;
	int channelIndex;
	QString configPath;
	int frameCount = 0;

	// output
	fftw_complex* resultFreqData = NULL;
	int freqDataLength = 0;
	int freqDataSampleRate = 48000;
	std::vector<FilterNode> plotNodes;
	double peakGain = 0.0;
	bool safetyBypassed = false;
	int invalidLineCount = 0;
	int overflowLineCount = 0;
	int latency = 0;
	double initializationTime = 0.0;
	double processingTime = 0.0;
	int processedFrames = 0;

	// internal (not protected by mutex)
	int lastFrameCount = -1;
	int lastChannelCount = -1;
	double* buf = NULL;
	double* buf2 = NULL;
	double* timeData = NULL;
	fftw_complex* freqData = NULL;
	fftw_plan planForward = NULL;
};
