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

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <QStandardItemModel>

#include "helpers/SafetyLimits.h"
#include "Editor/helpers/GUIHelper.h"
#include "BiQuadFilterGUI.h"
#include "ui_BiQuadFilterGUI.h"

static const double dialFreqSteps = 1000.0;
static const double dialFreqMin = SafetyLimits::MIN_FREQ_HZ;
static const double dialFreqMax = SafetyLimits::MAX_FREQ_HZ;
static const double dialQSteps = 1000.0;

BiQuadFilterGUI::BiQuadFilterGUI(BiQuadFilter* filter)
	: ui(new Ui::BiQuadFilterGUI)
{
	ui->setupUi(this);

	ui->freqDial->setFixedSize(GUIHelper::scale(QSize(100, 66)));
	ui->gainDial->setFixedSize(GUIHelper::scale(QSize(100, 66)));
	ui->qDial->setFixedSize(GUIHelper::scale(QSize(100, 66)));

	ui->freqSpinBox->setRange(SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ);
	ui->gainSpinBox->setRange(SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB);

	ui->typeComboBox->addItem(tr("Peaking filter"), BiQuad::PEAKING);
	ui->typeComboBox->addItem(tr("Low-pass filter"), BiQuad::LOW_PASS);
	ui->typeComboBox->addItem(tr("High-pass filter"), BiQuad::HIGH_PASS);
	ui->typeComboBox->addItem(tr("Band-pass filter"), BiQuad::BAND_PASS);
	ui->typeComboBox->addItem(tr("Low-shelf filter"), BiQuad::LOW_SHELF);
	ui->typeComboBox->addItem(tr("High-shelf filter"), BiQuad::HIGH_SHELF);
	ui->typeComboBox->addItem(tr("Notch filter"), BiQuad::NOTCH);
	ui->typeComboBox->addItem(tr("All-pass filter"), BiQuad::ALL_PASS);

	BiQuad::Type type = filter->getType();
	int typeIndex = ui->typeComboBox->findData(type);
	if (typeIndex != -1)
		ui->typeComboBox->setCurrentIndex(typeIndex);

	if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
		ui->freqComboBox->setCurrentIndex(filter->getIsCornerFreq() ? 1 : 0);

	ui->freqSpinBox->setValue(std::clamp(filter->getFreq(), SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ));
	ui->gainSpinBox->setValue(std::clamp(filter->getDbGain(), SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB));

	if (type == BiQuad::PEAKING)
		ui->qComboBox->setCurrentIndex(filter->getIsBandwidthOrS() ? 1 : 0);
	else if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS)
		ui->qComboBox->setCurrentIndex(filter->getBandwidthOrQOrS() == M_SQRT1_2 ? 0 : 1);
	else if ((type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF))
		ui->qComboBox->setCurrentIndex(filter->getIsBandwidthOrS() ? (filter->getBandwidthOrQOrS() == 0.9 || filter->getIsCornerFreq() ? 0 : 1) : (filter->getIsCornerFreq() ? 1 : 2));
	else if (type == BiQuad::NOTCH)
		ui->qComboBox->setCurrentIndex(filter->getBandwidthOrQOrS() == 30.0 ? 0 : 1);

	QChar currentMode = ui->qComboBox->currentData().toChar();
	double minQVal, maxQVal;
	getQModeRange(currentMode, minQVal, maxQVal);
	ui->qSpinBox->setRange(minQVal, maxQVal);

	double initialQSpin = filter->getBandwidthOrQOrS();
	if ((type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF) && filter->getIsBandwidthOrS() && filter->getBandwidthOrQOrS() != 0.9)
		initialQSpin = filter->getBandwidthOrQOrS() * 12.0;

	ui->qSpinBox->setValue(std::clamp(initialQSpin, minQVal, maxQVal));
	syncQDialFromSpinBox(ui->qSpinBox->value());
}

BiQuadFilterGUI::~BiQuadFilterGUI()
{
	delete ui;
}

void BiQuadFilterGUI::getQModeRange(QChar mode, double& minVal, double& maxVal) const
{
	if (mode == 'B')
	{
		minVal = SafetyLimits::MIN_BW_OCT_GUI;
		maxVal = SafetyLimits::MAX_BW_OCT_GUI;
	}
	else if (mode == 'S')
	{
		minVal = SafetyLimits::MIN_SLOPE_DB;
		maxVal = SafetyLimits::MAX_SLOPE_DB;
	}
	else
	{
		minVal = SafetyLimits::MIN_Q;
		maxVal = SafetyLimits::MAX_Q;
	}
}

void BiQuadFilterGUI::syncQDialFromSpinBox(double value)
{
	QChar mode = ui->qComboBox->currentData().toChar();
	double minVal, maxVal;
	getQModeRange(mode, minVal, maxVal);
	double clamped = std::clamp(value, minVal, maxVal);
	bool previousValue = ui->qDial->blockSignals(true);
	ui->qDial->setValue(round(dialQSteps * log(clamped / minVal) / log(maxVal / minVal)));
	ui->qDial->blockSignals(previousValue);
}

void BiQuadFilterGUI::store(QString& command, QString& parameters)
{
	BiQuad::Type type = (BiQuad::Type)ui->typeComboBox->currentData().toInt();
	QChar freqMode = ui->freqComboBox->currentData().toChar();
	QChar mode = ui->qComboBox->currentData().toChar();

	command = "Filter";
	parameters = "ON ";
	switch (type)
	{
	case BiQuad::PEAKING:
		parameters += "PK";
		break;
	case BiQuad::LOW_PASS:
		if (mode == 'F')
			parameters += "LP";
		else
			parameters += "LPQ";
		break;
	case BiQuad::HIGH_PASS:
		if (mode == 'F')
			parameters += "HP";
		else
			parameters += "HPQ";
		break;
	case BiQuad::BAND_PASS:
		parameters += "BP";
		break;
	case BiQuad::LOW_SHELF:
		if (freqMode == 'E' && mode != 'F')
			parameters += "LSC";
		else
			parameters += "LS";
		break;
	case BiQuad::HIGH_SHELF:
		if (freqMode == 'E' && mode != 'F')
			parameters += "HSC";
		else
			parameters += "HS";
		break;
	case BiQuad::NOTCH:
		parameters += "NO";
		break;
	case BiQuad::ALL_PASS:
		parameters += "AP";
		break;
	}

	if ((type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF) && mode == 'S')
		parameters += QString(" %1 dB").arg(std::clamp(ui->qSpinBox->value(), SafetyLimits::MIN_SLOPE_DB, SafetyLimits::MAX_SLOPE_DB));

	parameters += QString(" Fc %1 Hz").arg(std::clamp(ui->freqSpinBox->value(), SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ));

	if (type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF
		|| type == BiQuad::HIGH_SHELF)
		parameters += QString(" Gain %1 dB").arg(std::clamp(ui->gainSpinBox->value(), SafetyLimits::MIN_GAIN_DB, SafetyLimits::MAX_GAIN_DB));

	if (mode == 'Q')
		parameters += QString(" Q %1").arg(std::clamp(ui->qSpinBox->value(), SafetyLimits::MIN_Q, SafetyLimits::MAX_Q));
	else if (mode == 'B')
		parameters += QString(" BW Oct %1").arg(std::clamp(ui->qSpinBox->value(), SafetyLimits::MIN_BW_OCT_GUI, SafetyLimits::MAX_BW_OCT_GUI));
}

void BiQuadFilterGUI::on_freqDial_valueChanged(int value)
{
	ui->freqSpinBox->setValue(pow(dialFreqMax / dialFreqMin, value / dialFreqSteps) * dialFreqMin);
}

void BiQuadFilterGUI::on_freqSpinBox_valueChanged(double value)
{
	double clamped = std::clamp(value, dialFreqMin, dialFreqMax);
	bool previousValue = ui->freqDial->blockSignals(true);
	ui->freqDial->setValue(round(dialFreqSteps * log(clamped / dialFreqMin) / log(dialFreqMax / dialFreqMin)));
	ui->freqDial->blockSignals(previousValue);

	emit updateModel();
}

void BiQuadFilterGUI::on_qDial_valueChanged(int value)
{
	QChar mode = ui->qComboBox->currentData().toChar();
	double minVal, maxVal;
	getQModeRange(mode, minVal, maxVal);
	ui->qSpinBox->setValue(pow(maxVal / minVal, value / dialQSteps) * minVal);
}

void BiQuadFilterGUI::on_qSpinBox_valueChanged(double value)
{
	syncQDialFromSpinBox(value);

	emit updateModel();
}

void BiQuadFilterGUI::on_gainDial_valueChanged(int value)
{
	ui->gainSpinBox->setValue(value * 0.1);
}

void BiQuadFilterGUI::on_gainSpinBox_valueChanged(double value)
{
	bool previousValue = ui->gainDial->blockSignals(true);
	ui->gainDial->setValue(round(value / 0.1));
	ui->gainDial->blockSignals(previousValue);

	emit updateModel();
}

void BiQuadFilterGUI::on_typeComboBox_currentIndexChanged(int index)
{
	BiQuad::Type type = (BiQuad::Type)ui->typeComboBox->itemData(index).toInt();
	bool sameTypeCategory = getTypeCategory(previousType) == getTypeCategory(type);

	QChar freqMode = ui->freqComboBox->currentData().toChar();
	double freqValue = ui->freqSpinBox->value();
	ui->freqComboBox->clear();
	if (type == BiQuad::PEAKING || type == BiQuad::BAND_PASS || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF || type == BiQuad::NOTCH || type == BiQuad::ALL_PASS)
		ui->freqComboBox->addItem(tr("Center frequency"), 'E');
	if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
		ui->freqComboBox->addItem(tr("Corner frequency"), 'O');
	ui->freqComboBox->setEnabled(ui->freqComboBox->count() > 1);
	int freqIndex = ui->freqComboBox->findData(freqMode);
	if (freqIndex != -1 && sameTypeCategory)
	{
		ui->freqComboBox->setCurrentIndex(freqIndex);
		ui->freqSpinBox->setValue(freqValue);
	}
	else
	{
		freqMode = ui->freqComboBox->currentData().toChar();
	}

	QChar mode = ui->qComboBox->currentData().toChar();
	double qValue = ui->qSpinBox->value();
	ui->qComboBox->clear();
	if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS || type == BiQuad::NOTCH)
		ui->qComboBox->addItem(tr("Fixed Q"), 'F');
	else if ((type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF) && freqMode == 'E')
		ui->qComboBox->addItem(tr("Fixed S"), 'F');
	if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
		ui->qComboBox->addItem(tr("Slope"), 'S');
	if (type == BiQuad::PEAKING || type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::ALL_PASS || type == BiQuad::BAND_PASS || type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF || type == BiQuad::NOTCH)
		ui->qComboBox->addItem(tr("Q factor"), 'Q');
	if (type == BiQuad::PEAKING)
		ui->qComboBox->addItem(tr("Bandwidth"), 'B');
	ui->qComboBox->setEnabled(ui->qComboBox->count() > 1);
	int qIndex = ui->qComboBox->findData(mode);
	if (qIndex != -1 && sameTypeCategory)
	{
		ui->qComboBox->setCurrentIndex(qIndex);
		ui->qSpinBox->setValue(qValue);
	}
	else if (type == BiQuad::PEAKING || type == BiQuad::ALL_PASS)
	{
		ui->qSpinBox->setValue(10);
	}

	bool gainVisible = type == BiQuad::PEAKING || type == BiQuad::LOW_SHELF
		|| type == BiQuad::HIGH_SHELF;
	ui->gainDial->setVisible(gainVisible);
	ui->gainLabel->setVisible(gainVisible);
	ui->gainSpinBox->setVisible(gainVisible);

	previousType = type;

	emit updateModel();
}

void BiQuadFilterGUI::on_freqComboBox_currentIndexChanged(int index)
{
	BiQuad::Type type = (BiQuad::Type)ui->typeComboBox->currentData().toInt();
	QChar freqMode = ui->freqComboBox->itemData(index).toChar();
	if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
	{
		QChar mode = ui->qComboBox->currentData().toChar();
		double qValue = ui->qSpinBox->value();
		ui->qComboBox->clear();
		if (freqMode == 'E')
			ui->qComboBox->addItem(tr("Fixed S"), 'F');
		ui->qComboBox->addItem(tr("Slope"), 'S');
		ui->qComboBox->addItem(tr("Q factor"), 'Q');
		ui->qComboBox->setEnabled(ui->qComboBox->count() > 1);
		int qIndex = ui->qComboBox->findData(mode);
		if (qIndex != -1)
		{
			ui->qComboBox->setCurrentIndex(qIndex);
			ui->qSpinBox->setValue(qValue);
		}

		double dbGain = ui->gainSpinBox->value();
		double s = (ui->qSpinBox->value() / 12.0);
		if (mode == 'Q')
		{
			double q = ui->qSpinBox->value();
			double a = pow(10, dbGain / 40);
			double denom = (1.0 / (q * q) - 2.0) / (a + 1.0 / a) + 1.0;
			s = (denom > 1e-4) ? (1.0 / denom) : 1.0;
		}
		if (s < 1e-4)
			s = 1e-4;

		double centerFreqFactor = pow(10.0, std::abs(dbGain) / 80.0 / s);
		double freq = ui->freqSpinBox->value();
		if ((freqMode == 'E') == (type == BiQuad::LOW_SHELF))
			freq *= centerFreqFactor;
		else
			freq /= centerFreqFactor;
		ui->freqSpinBox->setValue(std::clamp(freq, SafetyLimits::MIN_FREQ_HZ, SafetyLimits::MAX_FREQ_HZ));
	}
}

void BiQuadFilterGUI::on_qComboBox_currentIndexChanged(int index)
{
	QChar mode = ui->qComboBox->currentData().toChar();
	double oldSpinVal = ui->qSpinBox->value();

	if (mode == 'F' || mode == 'Q')
		ui->qSpinBox->setSuffix("");
	else if (mode == 'B')
		ui->qSpinBox->setSuffix(" Oct");
	else if (mode == 'S')
		ui->qSpinBox->setSuffix(" dB/Oct");

	ui->qDial->setEnabled(mode != 'F');
	ui->qSpinBox->setEnabled(mode != 'F');

	double minVal, maxVal;
	getQModeRange(mode, minVal, maxVal);

	double targetVal = oldSpinVal;

	if (mode == 'F')
	{
		BiQuad::Type type = (BiQuad::Type)ui->typeComboBox->currentData().toInt();
		if (type == BiQuad::LOW_PASS || type == BiQuad::HIGH_PASS || type == BiQuad::BAND_PASS)
			targetVal = M_SQRT1_2;
		else if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF)
			targetVal = 0.9;
		else if (type == BiQuad::NOTCH)
			targetVal = 30.0;

		qIsBwOrS = type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF;
	}
	else if (qIsBwOrS && mode == 'Q')
	{
		BiQuad::Type type = (BiQuad::Type)ui->typeComboBox->currentData().toInt();
		if (type == BiQuad::LOW_SHELF || type == BiQuad::HIGH_SHELF) // from S
		{
			double s = oldSpinVal;
			if (s != 0.9)
				s /= 12.0;
			double a = pow(10.0, ui->gainSpinBox->value() / 40.0);
			double rad = std::max(1e-6, (a + 1.0 / a) * (1.0 / std::max(1e-4, s) - 1.0) + 2.0);
			targetVal = 1.0 / sqrt(rad);
		}
		else // from BW
		{
			double n = std::max(1e-4, oldSpinVal);
			double p2n = pow(2.0, n);
			targetVal = sqrt(p2n) / std::max(1e-6, p2n - 1.0);
		}
		qIsBwOrS = false;
	}
	else if (!qIsBwOrS && mode == 'B')
	{
		double q = std::max(1e-4, oldSpinVal);
		targetVal = 2.0 / M_LN2 * asinh(1.0 / (2.0 * q));
		qIsBwOrS = true;
	}
	else if (!qIsBwOrS && mode == 'S')
	{
		double q = std::max(1e-4, oldSpinVal);
		double a = pow(10.0, ui->gainSpinBox->value() / 40.0);
		double denom = (1.0 / (q * q) - 2.0) / (a + 1.0 / a) + 1.0;
		double s = (denom > 1.0) ? (1.0 / denom) : 1.0;
		targetVal = s * 12.0;
		qIsBwOrS = true;
	}
	else if (mode == 'S')
	{
		targetVal = 12.0;
		qIsBwOrS = true;
	}

	bool prevBlock = ui->qSpinBox->blockSignals(true);
	ui->qSpinBox->setRange(minVal, maxVal);
	ui->qSpinBox->setValue(std::clamp(targetVal, minVal, maxVal));
	ui->qSpinBox->blockSignals(prevBlock);
	syncQDialFromSpinBox(ui->qSpinBox->value());

	emit updateModel();
}

int BiQuadFilterGUI::getTypeCategory(BiQuad::Type type)
{
	switch (type)
	{
	case BiQuad::PEAKING:
		return 0;
	case BiQuad::LOW_PASS:
	case BiQuad::HIGH_PASS:
	case BiQuad::BAND_PASS:
		return 1;
	case BiQuad::LOW_SHELF:
	case BiQuad::HIGH_SHELF:
		return 2;
	case BiQuad::NOTCH:
		return 3;
	case BiQuad::ALL_PASS:
		return 4;
	}

	return -1;
}
