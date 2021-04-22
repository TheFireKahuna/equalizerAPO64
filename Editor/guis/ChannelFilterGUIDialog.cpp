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
#include <Ks.h>
#include <KsMedia.h>

#include "Editor/helpers/DPIHelper.h"
#include "Editor/helpers/GUIChannelHelper.h"
#include "ChannelFilterGUIDialog.h"
#include "ui_ChannelFilterGUIDialog.h"

#define PROPERTY_POSITION "CHANNEL_POSITION"
#define PROPERTY_NAME "CHANNEL_NAME"

using namespace std;

ChannelFilterGUIDialog::ChannelFilterGUIDialog(QWidget* parent, QStringList selectedChannels, int selectedChannelMask, const vector<wstring>& channelNames)
	: QDialog(parent),
	ui(new Ui::ChannelFilterGUIDialog)
{
	ui->setupUi(this);
	resize(DPIHelper::scale(QSize(355, 329)));

	ui->centerCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_FRONT_CENTER);
	ui->leftCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_FRONT_LEFT);
	ui->rightCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_FRONT_RIGHT);
	ui->sideLeftCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_SIDE_LEFT);
	ui->sideRightCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_SIDE_RIGHT);
	ui->rearLeftCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_BACK_LEFT);
	ui->rearRightCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_BACK_RIGHT);
	ui->rearCenterCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_BACK_CENTER);
	ui->subWooferCheckBox->setProperty(PROPERTY_POSITION, SPEAKER_LOW_FREQUENCY);

	ui->centerCheckBox->setProperty(PROPERTY_NAME, "C");
	ui->leftCheckBox->setProperty(PROPERTY_NAME, "L");
	ui->rightCheckBox->setProperty(PROPERTY_NAME, "R");
	ui->sideLeftCheckBox->setProperty(PROPERTY_NAME, "SL");
	ui->sideRightCheckBox->setProperty(PROPERTY_NAME, "SR");
	ui->rearLeftCheckBox->setProperty(PROPERTY_NAME, "RL");
	ui->rearRightCheckBox->setProperty(PROPERTY_NAME, "RR");
	ui->rearCenterCheckBox->setProperty(PROPERTY_NAME, "RC");
	ui->subWooferCheckBox->setProperty(PROPERTY_NAME, "LFE");

	checkBoxes.append(ui->centerCheckBox);
	checkBoxes.append(ui->leftCheckBox);
	checkBoxes.append(ui->rightCheckBox);
	checkBoxes.append(ui->sideLeftCheckBox);
	checkBoxes.append(ui->sideRightCheckBox);
	checkBoxes.append(ui->rearLeftCheckBox);
	checkBoxes.append(ui->rearRightCheckBox);
	checkBoxes.append(ui->rearCenterCheckBox);
	checkBoxes.append(ui->subWooferCheckBox);

	vector<wstring> remainingChannelNames = channelNames;
	QStringList remainingSelectedChannels = selectedChannels;
	if (selectedChannels.contains("ALL"))
	{
		ui->allChannelsCheckBox->setChecked(true);
		remainingSelectedChannels.clear();
	}
	int channelIndex = 0;
	for (QCheckBox* checkBox : checkBoxes)
	{
		int channelPos = checkBox->property(PROPERTY_POSITION).toInt();
		QString channelName = checkBox->property(PROPERTY_NAME).toString();
		bool exists = selectedChannelMask & channelPos;
		checkBox->setVisible(exists);
		if (exists)
		{
			channelIndex++;
			bool checked = false;
			remainingChannelNames.erase(remove(remainingChannelNames.begin(), remainingChannelNames.end(), channelName.toStdWString()), remainingChannelNames.end());
			if (remainingSelectedChannels.removeOne(channelName))
				checked = true;
			else if (channelName == "LFE" && remainingSelectedChannels.removeOne("SUB"))
				checked = true;

			if (remainingSelectedChannels.removeOne(QString(channelIndex)))
				checked = true;
			checkBox->setChecked(checked);
		}
		else
		{
			checkBox->setChecked(false);
		}
	}

	ui->listWidget->clear();
	for (wstring channelName : remainingChannelNames)
	{
		QListWidgetItem* item = new QListWidgetItem(QString::fromStdWString(channelName));
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		if (remainingSelectedChannels.removeOne(QString::fromStdWString(channelName)))
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
		ui->listWidget->addItem(item);
	}

	for (QString c : remainingSelectedChannels)
	{
		QListWidgetItem* item = new QListWidgetItem(c);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked);
		ui->listWidget->addItem(item);
	}

	ui->stackedWidget->setCurrentIndex(selectedChannelMask & SPEAKER_BACK_CENTER ? 1 : 0);
}

ChannelFilterGUIDialog::~ChannelFilterGUIDialog()
{
	delete ui;
}

void ChannelFilterGUIDialog::on_addButton_clicked()
{
	QListWidgetItem* item = new QListWidgetItem("");
	item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	item->setCheckState(Qt::Checked);
	ui->listWidget->addItem(item);
	ui->listWidget->editItem(item);
}

void ChannelFilterGUIDialog::on_removeButton_clicked()
{
	qDeleteAll(ui->listWidget->selectedItems());
}

QStringList ChannelFilterGUIDialog::getSelectedChannels() const
{
	QStringList selectedChannels;

	if (ui->allChannelsCheckBox->isChecked())
	{
		selectedChannels.append("ALL");
	}
	else
	{
		for (QCheckBox* checkBox : checkBoxes)
		{
			if (checkBox->isChecked())
			{
				QString channelName = checkBox->property(PROPERTY_NAME).toString();
				selectedChannels.append(channelName);
			}
		}

		for (int i = 0; i < ui->listWidget->count(); i++)
		{
			QListWidgetItem* item = ui->listWidget->item(i);
			if (item->checkState() == Qt::Checked)
			{
				QString text = item->text().trimmed();
				if (text.length() > 0)
					selectedChannels.append(text.toUpper());
			}
		}
	}

	return selectedChannels;
}

void ChannelFilterGUIDialog::on_allChannelsCheckBox_toggled(bool checked)
{
	ui->positionsGroupBox->setEnabled(!checked);
	ui->additionalChannelsGroupBox->setEnabled(!checked);
}
