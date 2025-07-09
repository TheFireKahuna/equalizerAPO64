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

#pragma once

#include <DeviceAPOInfo.h>
#include <QtWidgets/QDialog>
#include "DeviceTestThread.h"
#include "OpacityIconEngine.h"
#include "ui_DeviceTestDialog.h"

class DeviceTestDialog : public QDialog
{
	Q_OBJECT

public:
	DeviceTestDialog(QWidget* parent = nullptr);
	std::vector<std::shared_ptr<DeviceAPOInfo>> filterDevices(const std::vector<std::shared_ptr<AbstractAPOInfo>>& devices);
	void addDevices(std::vector<std::shared_ptr<DeviceAPOInfo>>& devices, QTreeWidgetItem* parentNode);

protected:
	__override void closeEvent(QCloseEvent* event);

private:
	Ui::DeviceTestDialogClass ui;

	QHash<QString, QTreeWidgetItem*> itemMap;
	QIcon animatedIcon;
	OpacityIconEngine* animatedIconEngine;
	DeviceTestThread* thread = nullptr;

	void log(const QString& message);
	void logError(const QString& message);
	void showErrorDialog(const QString& message);
	void setItemStatus(const QString& guid, bool postMix, ItemStatusType statusType);
	void animateIcon(const QVariant& value);
	void onThreadFinished();
	void onAbort(const QString& message, int code);
};
