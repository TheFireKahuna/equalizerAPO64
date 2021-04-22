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

#include "Editor/helpers/DPIHelper.h"
#include "CommentFilterGUI.h"
#include "ui_CommentFilterGUI.h"

using namespace std;

CommentFilterGUI::CommentFilterGUI(IFilterGUI* child, bool isComment)
{
	ui = new Ui::CommentFilterGUI;
	ui->setupUi(this);

	this->child = child;
	connect(child, SIGNAL(updateModel()), this, SIGNAL(updateModel()));
	connect(child, SIGNAL(updateChannels()), this, SIGNAL(updateChannels()));

	ui->horizontalLayout->removeItem(ui->horizontalSpacer);
	ui->horizontalLayout->addWidget(child);

	ui->actionPowerOn->setChecked(!isComment);
	ui->toolBar->setIconSize(DPIHelper::scale(QSize(24, 24)));
	ui->toolBar->addAction(ui->actionPowerOn);
	ui->toolBar->updateMaximumHeight();

	child->setEnabled(!isComment);
}

CommentFilterGUI::~CommentFilterGUI()
{
	delete ui;
}

void CommentFilterGUI::configureChannels(vector<wstring>& channelNames)
{
	if (ui->actionPowerOn->isChecked())
	{
		child->configureChannels(channelNames);
	}
	else
	{
		// prevent modification of channel names
		vector<wstring> copy = channelNames;
		child->configureChannels(copy);
	}
}

void CommentFilterGUI::store(QString& command, QString& parameters)
{
	child->store(command, parameters);

	if (!ui->actionPowerOn->isChecked())
		command = "# " + command;
}

void CommentFilterGUI::loadPreferences(const QVariantMap& prefs)
{
	child->loadPreferences(prefs);
}

void CommentFilterGUI::storePreferences(QVariantMap& prefs)
{
	child->storePreferences(prefs);
}

void CommentFilterGUI::on_actionPowerOn_toggled(bool checked)
{
	child->setEnabled(checked);

	emit updateModel();
}
