/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

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

#include <QTimer>

#include "Editor/IFilterGUI.h"
#include "filters/loudnessCorrection/VolumeController.h"

namespace Ui {
class LoudnessCorrectionFilterGUI;
}

class LoudnessCorrectionFilterGUI : public IFilterGUI
{
	Q_OBJECT

public:
	explicit LoudnessCorrectionFilterGUI(double refLevel, double refOffset, double att);
	~LoudnessCorrectionFilterGUI();

	void store(QString& command, QString& parameters) override;

private slots:
	void on_refLevelSpinBox_valueChanged(int arg1);
	void on_refOffsetSpinBox_valueChanged(int arg1);
	void on_attDial_valueChanged(int value);
	void on_attSpinBox_valueChanged(double arg1);
	void on_calibrateButton_clicked();
	void updateVolume();

private:
	Ui::LoudnessCorrectionFilterGUI* ui;
	bool state = true;
	QTimer timer;
	VolumeController volumeController;
	double lastVolume = -1;
};
