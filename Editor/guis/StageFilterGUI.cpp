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

#include "StageFilterGUI.h"
#include "ui_StageFilterGUI.h"

StageFilterGUI::StageFilterGUI(const QString& parameters)
	: ui(new Ui::StageFilterGUI)
{
	ui->setupUi(this);

	QStringList parts = parameters.toLower().split(' ');
	ui->preMixCheckBox->setChecked(parts.contains("pre-mix"));
	ui->postMixCheckBox->setChecked(parts.contains("post-mix"));
	ui->captureCheckBox->setChecked(parts.contains("capture"));
}

StageFilterGUI::~StageFilterGUI()
{
	delete ui;
}

void StageFilterGUI::store(QString& command, QString& parameters)
{
	command = "Stage";
	QStringList list;
	if (ui->preMixCheckBox->isChecked())
		list.append("pre-mix");
	if (ui->postMixCheckBox->isChecked())
		list.append("post-mix");
	if (ui->captureCheckBox->isChecked())
		list.append("capture");

	parameters = list.join(' ');
}

void StageFilterGUI::on_preMixCheckBox_toggled(bool checked)
{
	emit updateModel();
}

void StageFilterGUI::on_postMixCheckBox_toggled(bool checked)
{
	emit updateModel();
}

void StageFilterGUI::on_captureCheckBox_toggled(bool checked)
{
	emit updateModel();
}
