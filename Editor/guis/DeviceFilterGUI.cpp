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

#include <string>
#include "DeviceFilterGUI.h"
#include "DeviceFilterGUIDialog.h"
#include <filters/DeviceFilterFactory.h>
#include <VoicemeeterAPOInfo.h>
#include "ui_DeviceFilterGUI.h"

using namespace std;

DeviceFilterGUI::DeviceFilterGUI(DeviceFilterGUIFactory* factory)
	: ui(new Ui::DeviceFilterGUI)
{
	ui->setupUi(this);
	this->factory = factory;
}

DeviceFilterGUI::~DeviceFilterGUI()
{
	delete ui;
}

void DeviceFilterGUI::load(const QString& parameters)
{
	pattern = parameters;

	ui->treeWidget->clear();

	QStringList labels;
	labels.append(tr("Type"));
	labels.append(tr("Connection"));
	labels.append(tr("Device"));
	labels.append(tr("State"));
	ui->treeWidget->setHeaderLabels(labels);

	QTreeWidgetItem* lastItem = NULL;

	if (parameters.trimmed() == "all")
	{
		QStringList values;
		values.append("");
		values.append(tr("All devices"));
		values.append("");

		lastItem = new QTreeWidgetItem(ui->treeWidget, values);
	}
	else
	{
		const QList<shared_ptr<AbstractAPOInfo> >& devices = factory->getDevices();
		bool anyInstalled = false;
		for (const shared_ptr<AbstractAPOInfo>& apoInfo : devices)
		{
			if (apoInfo->isInstalled() && DeviceFilterFactory::matchDevice(apoInfo->getDeviceString(), pattern.toStdWString()))
			{
				anyInstalled = true;
				break;
			}
		}

		for (const shared_ptr<AbstractAPOInfo>& apoInfo : devices)
		{
			if (anyInstalled && !apoInfo->isInstalled())
				continue;

			if (DeviceFilterFactory::matchDevice(apoInfo->getDeviceString(), pattern.toStdWString()))
			{
				QStringList values;
				if (apoInfo->isInput())
					values.append(tr("Capture"));
				else
					values.append(tr("Playback"));
				values.append(QString::fromStdWString(apoInfo->getConnectionName()));
				values.append(QString::fromStdWString(apoInfo->getDeviceName()));
				QString state;
				if (apoInfo->isInstalled())
					state = tr("APO installed");
				else
					state = tr("APO not installed");
				VoicemeeterAPOInfo* voicemeeterInfo = dynamic_cast<VoicemeeterAPOInfo*>(apoInfo.get());
				if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
					state += ", " + tr("Voicemeeter was uninstalled");
				values.append(state);
				lastItem = new QTreeWidgetItem(ui->treeWidget, values);
			}
		}

		if (ui->treeWidget->topLevelItemCount() == 0)
		{
			QStringList values;
			values.append("");
			values.append(tr("No device matched") + " \"" + pattern.trimmed() + "\"");
			values.append("");

			QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget, values);
			item->setForeground(1, QBrush(Qt::red));
			lastItem = item;
		}
	}

	for (int i = 0; i < ui->treeWidget->columnCount(); i++)
		ui->treeWidget->resizeColumnToContents(i);

	int headerHeight = ui->treeWidget->header()->height();
	int itemAreaHeight = 0;
	if (lastItem != NULL)
		itemAreaHeight = ui->treeWidget->visualItemRect(lastItem).bottom() + 1;
	ui->treeWidget->setFixedHeight(headerHeight + itemAreaHeight + 3);
}

void DeviceFilterGUI::store(QString& command, QString& parameters)
{
	command = "Device";
	parameters = pattern;
}

void DeviceFilterGUI::on_pushButton_clicked()
{
	DeviceFilterGUIDialog dialog(this, factory, pattern);
	if (dialog.exec() == QDialog::Accepted)
	{
		load(dialog.getPattern());
		emit updateModel();
	}
}
