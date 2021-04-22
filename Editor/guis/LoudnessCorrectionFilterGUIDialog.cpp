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

#include <QFile>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "Editor/helpers/QtSndfileHandle.h"

#include "LoudnessCorrectionFilterGUIDialog.h"
#include "ui_LoudnessCorrectionFilterGUIDialog.h"

LoudnessCorrectionFilterGUIDialog::LoudnessCorrectionFilterGUIDialog(QWidget* parent)
	: QDialog(parent),
	ui(new Ui::LoudnessCorrectionFilterGUIDialog)
{
	ui->setupUi(this);
}

LoudnessCorrectionFilterGUIDialog::~LoudnessCorrectionFilterGUIDialog()
{
	on_stopButton_clicked();
	delete ui;
}

int LoudnessCorrectionFilterGUIDialog::getMeasuredLevel()
{
	return ui->levelSpinBox->value();
}

void LoudnessCorrectionFilterGUIDialog::on_playButton_clicked()
{
	if (buffer.size() > 0)
		on_stopButton_clicked();

	QFile file(":/sounds/pinkNoise.flac");
	file.open(QIODevice::ReadOnly);
	QtSndfileHandle fileHandle(file, SFM_READ);

	buffer.open(QIODevice::WriteOnly);
	{
		QtSndfileHandle bufferHandle(buffer, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_24, 2, fileHandle.samplerate());

		bool leftEnabled = ui->leftRadioButton->isChecked() || ui->bothRadioButton->isChecked();
		bool rightEnabled = ui->rightRadioButton->isChecked() || ui->bothRadioButton->isChecked();

		int buf[1024];
		int buf2[2 * ARRAYSIZE(buf)];
		int samplesRead;
		while ((samplesRead = fileHandle.read(buf, ARRAYSIZE(buf))) > 0)
		{
			for (int i = 0; i < samplesRead; i++)
			{
				buf2[2 * i] = leftEnabled ? buf[i] : 0;
				buf2[2 * i + 1] = rightEnabled ? buf[i] : 0;
			}
			bufferHandle.write(buf2, 2 * samplesRead);
		}
	}
	buffer.close();

	PlaySoundA(buffer.data().data(), NULL, SND_MEMORY | SND_ASYNC | SND_LOOP);
}

void LoudnessCorrectionFilterGUIDialog::on_stopButton_clicked()
{
	if (buffer.size() == 0)
		return;

	PlaySoundA(NULL, NULL, 0);
	buffer.buffer().clear();
}

void LoudnessCorrectionFilterGUIDialog::on_leftRadioButton_toggled(bool checked)
{
	if (checked && buffer.size() > 0)
		on_playButton_clicked();
}

void LoudnessCorrectionFilterGUIDialog::on_rightRadioButton_toggled(bool checked)
{
	if (checked && buffer.size() > 0)
		on_playButton_clicked();
}

void LoudnessCorrectionFilterGUIDialog::on_bothRadioButton_toggled(bool checked)
{
	if (checked && buffer.size() > 0)
		on_playButton_clicked();
}
