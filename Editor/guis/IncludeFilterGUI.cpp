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

#include "helpers/RegistryHelper.h"
#include "IncludeFilterGUI.h"
#include "ui_IncludeFilterGUI.h"

IncludeFilterGUI::IncludeFilterGUI(FilterTable* filterTable, const QString& path)
	: ui(new Ui::IncludeFilterGUI), filterTable(filterTable)
{
	ui->setupUi(this);

	ui->pathLineEdit->setText(path);

	updateFileInfo();
}

IncludeFilterGUI::~IncludeFilterGUI()
{
	delete ui;
}

void IncludeFilterGUI::store(QString& command, QString& parameters)
{
	command = "Include";
	parameters = ui->pathLineEdit->text();
}

void IncludeFilterGUI::on_selectFileToolButton_clicked()
{
	QFileInfo fileInfo(filterTable->getConfigPath());
	QDir configDir = fileInfo.absoluteDir();
	QString path = ui->pathLineEdit->text();
	if (path.length() > 0)
		fileInfo.setFile(configDir, path);

	QFileDialog dialog(this, tr("Include file"), fileInfo.absolutePath(), "*.txt");
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("E-APO configurations (*.txt)"));
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

void IncludeFilterGUI::on_pathLineEdit_editingFinished()
{
	updateFileInfo();

	emit updateModel();
}

void IncludeFilterGUI::on_openFileToolButton_clicked()
{
	QFileInfo fileInfo(filterTable->getConfigPath());
	QDir configDir = fileInfo.absoluteDir();
	QString path = ui->pathLineEdit->text();
	if (path.length() > 0)
	{
		fileInfo.setFile(configDir, path);

		filterTable->openConfig(fileInfo.absoluteFilePath());
	}
}

void IncludeFilterGUI::updateFileInfo()
{
	QString error = "";

	QString path = ui->pathLineEdit->text();
	if (path.length() == 0)
	{
		error = tr("No file selected");
	}
	else
	{
		QFileInfo fileInfo(filterTable->getConfigPath());
		QDir configDir = fileInfo.absoluteDir();
		fileInfo.setFile(configDir, path);
		if (!fileInfo.exists())
		{
			error = tr("File not found");
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
				error = tr("The file is not readable for the audio service.\nChange the file permissions or copy the file to the config directory.");
		}
	}

	ui->errorLabel->setVisible(error.length() > 0);
	ui->errorLabel->setText(error);
}
