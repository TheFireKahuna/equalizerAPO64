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

#include "Editor/helpers/GUIHelper.h"
#include "DeviceFilterGUIDialog.h"
#include "ui_DeviceFilterGUIDialog.h"

#include <filters/DeviceFilterFactory.h>
#include <VoicemeeterAPOInfo.h>

using namespace std;

DeviceFilterGUIDialog::DeviceFilterGUIDialog(DeviceFilterGUI* gui, DeviceFilterGUIFactory* factory, const QString& pattern)
	: QDialog(gui),
	ui(new Ui::DeviceFilterGUIDialog)
{
	ui->setupUi(this);
	resize(GUIHelper::scale(QSize(500, 350)));

	bool all = pattern.trimmed() == "all";
	ui->allDevicesCheckBox->setChecked(all);

	QStringList labels;
	labels.append(tr("Connection"));
	labels.append(tr("Device"));
	labels.append(tr("State"));
	ui->treeWidget->setHeaderLabels(labels);
	QTreeWidgetItem* outputNode = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Playback devices")));
	outputNode->setExpanded(true);
	QTreeWidgetItem* inputNode = new QTreeWidgetItem(ui->treeWidget, QStringList(tr("Capture devices")));
	inputNode->setExpanded(true);
	const QList<shared_ptr<AbstractAPOInfo>>& devices = factory->getDevices();
	for (const shared_ptr<AbstractAPOInfo>& apoInfo : devices)
	{
		QStringList values;
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
		QTreeWidgetItem* item = new QTreeWidgetItem(apoInfo->isInput() ? inputNode : outputNode, values);

		bool matches = !all && DeviceFilterFactory::matchDevice(apoInfo->getDeviceString(), pattern.toStdWString());
		item->setCheckState(0, matches ? Qt::Checked : Qt::Unchecked);
		item->setData(0, Qt::UserRole, QVariant::fromValue(apoInfo));
		item->setHidden(!matches && !apoInfo->isInstalled() && ui->showOnlyInstalledCheckBox->isChecked());
		if (!apoInfo->isInstalled())
			for (int i = 0; i < ui->treeWidget->columnCount(); i++)
				item->setForeground(i, QBrush(Qt::gray));
	}

	for (int i = 0; i < ui->treeWidget->columnCount(); i++)
		ui->treeWidget->resizeColumnToContents(i);
}

DeviceFilterGUIDialog::~DeviceFilterGUIDialog()
{
	delete ui;
}

QString DeviceFilterGUIDialog::getPattern()
{
	if (ui->allDevicesCheckBox->isChecked())
	{
		return "all";
	}
	else
	{
		QString pattern = "";
		for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
		{
			QTreeWidgetItem* groupItem = ui->treeWidget->topLevelItem(i);
			for (int j = 0; j < groupItem->childCount(); j++)
			{
				QTreeWidgetItem* item = groupItem->child(j);
				if (item->checkState(0) == Qt::Checked)
				{
					if (pattern != "")
						pattern += "; ";
					shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<shared_ptr<AbstractAPOInfo>>();
					pattern += QString::fromStdWString(apoInfo->getDeviceString());
				}
			}
		}

		return pattern;
	}
}

void DeviceFilterGUIDialog::on_allDevicesCheckBox_toggled(bool checked)
{
	ui->treeWidget->setEnabled(!checked);
}

void DeviceFilterGUIDialog::on_showOnlyInstalledCheckBox_toggled(bool checked)
{
	for (int i = 0; i < ui->treeWidget->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* groupItem = ui->treeWidget->topLevelItem(i);
		for (int j = 0; j < groupItem->childCount(); j++)
		{
			QTreeWidgetItem* item = groupItem->child(j);
			shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<shared_ptr<AbstractAPOInfo>>();
			item->setHidden(item->checkState(0) != Qt::Checked && !apoInfo->isInstalled() && checked);
		}
	}

	for (int i = 0; i < ui->treeWidget->columnCount(); i++)
		ui->treeWidget->resizeColumnToContents(i);
}
