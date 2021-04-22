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

#include <QFileDialog>
#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1
#include <sndfile.h>

#include "helpers/RegistryHelper.h"
#include "ConvolutionFilterGUI.h"
#include "ui_ConvolutionFilterGUI.h"

ConvolutionFilterGUI::ConvolutionFilterGUI(const QString& configPath, unsigned deviceSampleRate, const QString& path)
	: ui(new Ui::ConvolutionFilterGUI), deviceSampleRate(deviceSampleRate)
{
	ui->setupUi(this);

	this->configPath = configPath;
	ui->pathLineEdit->setText(path);

	updateFileInfo();
}

ConvolutionFilterGUI::~ConvolutionFilterGUI()
{
	delete ui;
}

void ConvolutionFilterGUI::store(QString& command, QString& parameters)
{
	command = "Convolution";
	parameters = ui->pathLineEdit->text();
}

void ConvolutionFilterGUI::on_selectFileToolButton_clicked()
{
	QFileInfo fileInfo(configPath);
	QDir configDir = fileInfo.absoluteDir();
	QString path = ui->pathLineEdit->text();
	if (path.length() > 0)
		fileInfo.setFile(configDir, path);

	QFileDialog dialog(this, tr("Select impulse response file"), fileInfo.absolutePath(), "*.wav;*.flac;*.ogg");
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("Impulse response (*.wav *.flac *.ogg)"));
	if (path.length() > 0)
		dialog.selectFile(fileInfo.fileName());
	if (dialog.exec() == QDialog::Accepted)
	{
		QString absolutePath = dialog.selectedFiles().first();
		QString relativePath = configDir.relativeFilePath(absolutePath);
		if (relativePath.startsWith("../../"))
			relativePath = absolutePath;
		ui->pathLineEdit->setText(QDir::toNativeSeparators(relativePath));
		updateFileInfo();

		emit updateModel();
	}
}

void ConvolutionFilterGUI::on_pathLineEdit_editingFinished()
{
	updateFileInfo();

	emit updateModel();
}

void ConvolutionFilterGUI::updateFileInfo()
{
	bool labelsVisible = true;
	QString error = "";

	QString path = ui->pathLineEdit->text();
	if (path.length() == 0)
	{
		error = tr("No file selected");
		labelsVisible = false;
	}
	else
	{
		QFileInfo fileInfo(configPath);
		QDir configDir = fileInfo.absoluteDir();
		fileInfo.setFile(configDir, path);
		if (!fileInfo.exists())
		{
			error = tr("File not found");
			labelsVisible = false;
		}
		else
		{
			path = QDir::toNativeSeparators(fileInfo.absoluteFilePath());

			ACCESS_MASK mask = GENERIC_READ;
			try
			{
				mask = RegistryHelper::getFileAccessForUser(path.toStdWString(), SECURITY_LOCAL_SERVICE_RID);
			}
			catch (RegistryException e)
			{
				// ignore
			}

			if ((mask & GENERIC_READ) != GENERIC_READ && (mask & FILE_GENERIC_READ) != FILE_GENERIC_READ)
			{
				error = tr("The file is not readable for the audio service.\nChange the file permissions or copy the file to the config directory.");
				labelsVisible = false;
			}
			else
			{
				SF_INFO info;
				SNDFILE* file = sf_wchar_open(path.toStdWString().c_str(), SFM_READ, &info);
				if (file == NULL)
				{
					error = tr("Unsupported file format");
					labelsVisible = false;
				}
				else
				{
					int sampleRate = info.samplerate;
					double length = info.frames * 1000.0 / sampleRate;

					ui->labelLengthValue->setText(tr("%0 ms (%1 samples)").arg(length).arg(info.frames));
					ui->labelSampleRateValue->setText(tr("%0 Hz").arg(sampleRate));
					sf_close(file);

					if (sampleRate != deviceSampleRate)
					{
						error = tr("The file sample rate does not match the device sample rate (%0 Hz)!\nSelect a different file or change the device configuration.").arg(deviceSampleRate);
					}
				}
			}
		}
	}

	ui->labelLength->setVisible(labelsVisible);
	ui->labelLengthValue->setVisible(labelsVisible);
	ui->labelSampleRate->setVisible(labelsVisible);
	ui->labelSampleRateValue->setVisible(labelsVisible);
	ui->labelError->setVisible(error.length() > 0);
	ui->labelError->setText(error);
}
