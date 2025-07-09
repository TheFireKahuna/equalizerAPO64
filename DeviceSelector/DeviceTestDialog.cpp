/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2024  Jonas Thedering

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

#include "stdafx.h"
#include <DeviceAPOInfo.h>
#include <helpers/RegistryHelper.h>
#include "DeviceTestDialog.h"
#include "OpacityIconEngine.h"

DeviceTestDialog::DeviceTestDialog(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	setWindowFlags(windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

	animatedIconEngine = new OpacityIconEngine(QIcon(":/icons/circle.svg"));
	animatedIcon = QIcon(animatedIconEngine);

	QVector<std::shared_ptr<DeviceAPOInfo>> devices;
	try
	{
		std::vector<std::shared_ptr<DeviceAPOInfo>> outputDevices = filterDevices(DeviceAPOInfo::loadAllInfos(false));
		if (!outputDevices.empty())
		{
			QTreeWidgetItem* outputNode = new QTreeWidgetItem(ui.deviceTreeWidget, QStringList(tr("Playback devices")));
			outputNode->setExpanded(true);
			addDevices(outputDevices, outputNode);
			devices.append(QVector<std::shared_ptr<DeviceAPOInfo>>(outputDevices.begin(), outputDevices.end()));
		}

		std::vector<std::shared_ptr<DeviceAPOInfo>> inputDevices = filterDevices(DeviceAPOInfo::loadAllInfos(true));
		if (!inputDevices.empty())
		{
			QTreeWidgetItem* inputNode = new QTreeWidgetItem(ui.deviceTreeWidget, QStringList(tr("Capture devices")));
			inputNode->setExpanded(true);
			addDevices(inputDevices, inputNode);
			devices.append(QVector<std::shared_ptr<DeviceAPOInfo>>(inputDevices.begin(), inputDevices.end()));
		}
	}
	catch (RegistryException e)
	{
		QMessageBox::critical(this, tr("Error while accessing the registry"), QString::fromStdWString(e.getMessage()));
	}

	for (int i = 0; i < ui.deviceTreeWidget->columnCount(); i++)
		ui.deviceTreeWidget->resizeColumnToContents(i);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

	adjustSize();

	// workaround for Qt 6 to not initially have scrollbars despite correct dialog size
	ui.deviceTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QTimer::singleShot(0, [&] {ui.deviceTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); });

	QVariantAnimation* animation = new QVariantAnimation(this);
	connect(animation, &QVariantAnimation::valueChanged, this, &DeviceTestDialog::animateIcon);
	animation->setStartValue(qreal(0.2));
	animation->setKeyValueAt(0.5, qreal(1.0));
	animation->setEndValue(qreal(0.2));
	animation->setLoopCount(-1);
	animation->setDuration(1000);
	animation->setEasingCurve(QEasingCurve::InOutQuad);
	animation->start();

	qRegisterMetaType<ItemStatusType>();

	thread = new DeviceTestThread(this, devices);
	connect(thread, &DeviceTestThread::log, this, &DeviceTestDialog::log);
	connect(thread, &DeviceTestThread::logError, this, &DeviceTestDialog::logError);
	connect(thread, &DeviceTestThread::showErrorDialog, this, &DeviceTestDialog::showErrorDialog);
	connect(thread, &DeviceTestThread::setItemStatus, this, &DeviceTestDialog::setItemStatus);
	connect(thread, &DeviceTestThread::finished, this, &DeviceTestDialog::onThreadFinished);
	connect(thread, &DeviceTestThread::abort, this, &DeviceTestDialog::onAbort);
	thread->start();
}

std::vector<std::shared_ptr<DeviceAPOInfo>> DeviceTestDialog::filterDevices(const std::vector<std::shared_ptr<AbstractAPOInfo>>& devices)
{
	std::vector<std::shared_ptr<DeviceAPOInfo>> filteredList;
	for (const std::shared_ptr<AbstractAPOInfo>& apoInfo : devices)
	{
		if (apoInfo->isInstalled())
		{
			std::shared_ptr<DeviceAPOInfo> deviceApoInfo = std::dynamic_pointer_cast<DeviceAPOInfo>(apoInfo);
			if (deviceApoInfo)
				filteredList.push_back(deviceApoInfo);
		}
	}

	return filteredList;
}

void DeviceTestDialog::addDevices(std::vector<std::shared_ptr<DeviceAPOInfo>>& devices, QTreeWidgetItem* parentNode)
{
	for (const std::shared_ptr<DeviceAPOInfo>& apoInfo : devices)
	{
		QStringList values;
		values.append(QString::fromStdWString(apoInfo->getConnectionName()));
		values.append(QString::fromStdWString(apoInfo->getDeviceName()));

		QIcon icon = animatedIcon;
		QString tooltip;
		if (apoInfo->isDisabled())
		{
			icon = QIcon(":/icons/circle.svg");
			tooltip = tr("Cannot test APO installation as device is disabled");
		}
		else if (apoInfo->isUnplugged())
		{
			icon = QIcon(":/icons/circle.svg");
			tooltip = tr("Cannot test APO installation as device is unplugged");
		}

		QTreeWidgetItem* item = new QTreeWidgetItem(parentNode, values);
		QMovie::supportedFormats();
		if (apoInfo->getSelectedInstallState().installPreMix)
		{
			item->setIcon(2, icon);
			item->setToolTip(2, tooltip);
		}
		if (apoInfo->getSelectedInstallState().installPostMix && !apoInfo->isInput())
		{
			item->setIcon(3, icon);
			item->setToolTip(3, tooltip);
		}

		itemMap.insert(QString::fromStdWString(apoInfo->getDeviceGuid()).toLower(), item);
	}
}

void DeviceTestDialog::closeEvent(QCloseEvent* event)
{
	if (thread != nullptr)
		thread->terminate();
}

void DeviceTestDialog::log(const QString& message)
{
	ui.statusOutputBox->append(message);
}

void DeviceTestDialog::logError(const QString& message)
{
	ui.statusOutputBox->append("<span style=\"color:red\">" + message + "</span>");
}

void DeviceTestDialog::showErrorDialog(const QString& message)
{
	QMessageBox::critical(this, tr("Error"), message);
}

void DeviceTestDialog::setItemStatus(const QString& guid, bool postMix, ItemStatusType statusType)
{
	QTreeWidgetItem* item = itemMap.value(guid.toLower());
	if (item != nullptr)
	{
		QIcon icon;
		QString tooltip;
		switch (statusType)
		{
		case ItemStatusType::waiting:
			icon = animatedIcon;
			break;
		case ItemStatusType::success:
			icon = QIcon(":/icons/circle-check.svg");
			break;
		case ItemStatusType::warning:
			icon = QIcon(":/icons/circle-warning.svg");
			tooltip = tr("Equalizer APO works but original APO could not be initialized. Maybe unset \"Use original APO\" in troubleshooting options.");
			break;
		case ItemStatusType::error:
			icon = QIcon(":/icons/circle-error.svg");
			break;
		}
		item->setIcon(postMix ? 3 : 2, icon);
		item->setToolTip(postMix ? 3 : 2, tooltip);
	}
}

void DeviceTestDialog::animateIcon(const QVariant& value)
{
	qreal opacity = value.value<qreal>();
	animatedIconEngine->setOpacity(opacity);

	ui.deviceTreeWidget->viewport()->update();
}

void DeviceTestDialog::onThreadFinished()
{
	ui.buttonBox->setEnabled(true);
}

void DeviceTestDialog::onAbort(const QString& message, int code)
{
	QMessageBox::critical(this, tr("Error"), message);
	done(code);
}
