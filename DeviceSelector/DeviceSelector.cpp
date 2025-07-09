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
#include <helpers/ServiceHelper.h>
#include <VoicemeeterAPOInfo.h>
#include "DeviceTestDialog.h"
#include "version.h"
#include "DeviceSelector.h"

DeviceSelector::DeviceSelector(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	setWindowFlags(windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

	QString version = QString("%0.%1").arg(MAJOR).arg(MINOR);
	if (REVISION != 0)
		version += QString(".%0").arg(REVISION);
	setWindowTitle(QString("Equalizer APO %0 Device Selector").arg(version));

	try
	{
		QTreeWidgetItem* outputNode = new QTreeWidgetItem(ui.deviceTreeWidget, QStringList(tr("Playback devices")));
		outputNode->setExpanded(true);
		std::vector<std::shared_ptr<AbstractAPOInfo>> outputDevices = DeviceAPOInfo::loadAllInfos(false);
		addDevices(outputDevices, outputNode);

		QTreeWidgetItem* inputNode = new QTreeWidgetItem(ui.deviceTreeWidget, QStringList(tr("Capture devices")));
		inputNode->setExpanded(true);
		std::vector<std::shared_ptr<AbstractAPOInfo>> inputDevices = DeviceAPOInfo::loadAllInfos(true);
		addDevices(inputDevices, inputNode);
	}
	catch (RegistryException e)
	{
		QMessageBox::critical(this, tr("Error while accessing the registry"), QString::fromStdWString(e.getMessage()));
	}

	for (int i = 0; i < ui.deviceTreeWidget->columnCount(); i++)
		ui.deviceTreeWidget->resizeColumnToContents(i);

	ui.deviceTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	if (!RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
	{
		ui.installModeComboBox->removeItem(2);
		ui.installModeComboBox->removeItem(1);
	}

	updateButtons();

	connect(ui.deviceTreeWidget, &QTreeWidget::itemChanged, this, &DeviceSelector::onDeviceToggled);
	connect(ui.deviceTreeWidget, &QTreeWidget::itemSelectionChanged, this, &DeviceSelector::onDeviceSelectionChanged);
	connect(ui.deviceTreeWidget, &QTreeWidget::customContextMenuRequested, this, &DeviceSelector::onDeviceContextMenuRequested);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(this, &QDialog::accepted, this, &DeviceSelector::onDialogAccepted);
	connect(this, &QDialog::rejected, this, &DeviceSelector::onDialogRejected);
	connect(ui.copyDeviceCommandAction, &QAction::triggered, this, &DeviceSelector::onCopyDeviceCommandClicked);
	connect(ui.troubleshootingGroupBox, &QGroupBox::toggled, this, &DeviceSelector::onTroubleShootingToggled);
	connect(ui.installPreMixCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.installPostMixCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.useOriginalAPOPreMixCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.useOriginalAPOPostMixCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.installModeComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.allowSilentBufferCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);
	connect(ui.autoCheckBox, &QCheckBox::clicked, this, &DeviceSelector::onTroubleShootingOptionChanged);

	ui.troubleshootingGroupBox->setChecked(false);
	adjustSize();

	// workaround for Qt 6 to not initially have scrollbars despite correct dialog size
	ui.deviceTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	QTimer::singleShot(0, [&] {ui.deviceTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); });

	bool fixedAudioDG = !DeviceAPOInfo::checkProtectedAudioDG(true);
	bool fixedRegistration = !DeviceAPOInfo::checkAPORegistration(true);
	if (fixedAudioDG || fixedRegistration)
	{
		QMessageBox::information(this, tr("Info"), tr("A registry value that is required for the operation of Equalizer APO was not set correctly. "
			"This might have been caused by a driver installation or uninstallation. The value has been corrected. A reboot may be required so that the changes can take effect."));
		askForReboot = true;
	}
}

void DeviceSelector::addDevices(std::vector<std::shared_ptr<AbstractAPOInfo>>& devices, QTreeWidgetItem* parentNode)
{
	for (const std::shared_ptr<AbstractAPOInfo>& apoInfo : devices)
	{
		QStringList values;
		values.append(QString::fromStdWString(apoInfo->getConnectionName()));
		values.append(QString::fromStdWString(apoInfo->getDeviceName()));

		VoicemeeterAPOInfo* voicemeeterInfo = dynamic_cast<VoicemeeterAPOInfo*>(apoInfo.get());
		bool checked = false;
		if (apoInfo->isInstalled())
		{
			if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
				checked = false;
			else
				checked = true;
		}
		QString state = getStateText(apoInfo, checked);

		values.append(state);
		QTreeWidgetItem* item = new QTreeWidgetItem(parentNode, values);

		item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
		item->setData(0, Qt::UserRole, QVariant::fromValue(apoInfo));
	}
}

void DeviceSelector::onDeviceSelectionChanged()
{
	updateButtons();
}

void DeviceSelector::onDeviceToggled(QTreeWidgetItem* item)
{
	updateList(item);
	updateButtons();
}

void DeviceSelector::onDeviceContextMenuRequested(const QPoint& pos)
{
	QMenu menu(this);
	menu.addAction(ui.copyDeviceCommandAction);
	menu.exec(ui.deviceTreeWidget->mapToGlobal(pos));
}

void DeviceSelector::onDialogAccepted()
{
	bool deviceUpdated = false;

	for (int index = 0; index < ui.deviceTreeWidget->topLevelItemCount(); index++)
	{
		QTreeWidgetItem* topItem = ui.deviceTreeWidget->topLevelItem(index);
		for (int i = 0; i < topItem->childCount(); i++)
		{
			QTreeWidgetItem* item = topItem->child(i);
			std::shared_ptr<AbstractAPOInfo> info = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
			bool checked = item->checkState(0) == Qt::Checked;

			try
			{
				DeviceAPOInfo* deviceInfo = dynamic_cast<DeviceAPOInfo*>(info.get());
				if (checked && !info->isInstalled())
				{
					info->install();
					if (deviceInfo != NULL)
						deviceUpdated = true;
				}
				else if (!checked && info->isInstalled())
				{
					info->uninstall();
					if (deviceInfo != NULL)
						deviceUpdated = true;
				}
				else if (checked && (info->canBeUpgraded() || info->hasChanges() || info->isEnhancementsDisabled()))
				{
					info->reinstall();
					if (deviceInfo != NULL)
						deviceUpdated = true;
				}
			}
			catch (RegistryException e)
			{
				QMessageBox::critical(this, tr("Error while accessing the registry"), QString::fromStdWString(e.getMessage()));
			}
		}
	}

	VoicemeeterAPOInfo::ensureVoicemeeterClientRunning();

	finish(deviceUpdated);
}

void DeviceSelector::onDialogRejected()
{
	if (hasUpgrades())
	{
		if (QMessageBox::warning(this, tr("Upgrades available"), tr("The APO installation of some devices should be upgraded. Do you really want to cancel?"),
			QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No)) == QMessageBox::No)
			return;
	}

	finish(false);
}

void DeviceSelector::finish(bool deviceUpdated)
{
	int dialogResult = 0;
	if (QCoreApplication::instance()->arguments().contains("/i")
		|| deviceUpdated || askForReboot)
	{
		DeviceTestDialog testDialog;
		dialogResult = testDialog.exec();
	}

	int returnCode = 0;
	if (QCoreApplication::instance()->arguments().contains("/i"))
	{
		QMessageBox::information(this, tr("Info"), tr("This dialog can be reopened anytime by launching Device Selector from the start menu."));
		if (dialogResult == -1)
			returnCode = 1;
	}
	else if (dialogResult == -1)
	{
		if (QMessageBox::question(this, tr("Reboot"), tr("To apply the changes, Windows should be rebooted. Reboot now?")) == QMessageBox::Yes)
		{
			HANDLE tokenHandle;
			if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
			{
				LUID luid;
				if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid))
				{
					TOKEN_PRIVILEGES tp;
					tp.PrivilegeCount = 1;
					tp.Privileges[0].Luid = luid;
					tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

					if (AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
						InitiateShutdownW(NULL, NULL, 0, SHUTDOWN_RESTART | SHUTDOWN_GRACE_OVERRIDE, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE);
				}

				CloseHandle(tokenHandle);
			}
		}
	}

	QCoreApplication::exit(returnCode);
}

void DeviceSelector::onCopyDeviceCommandClicked()
{
	QString command = "Device: ";

	QList<QTreeWidgetItem*> list = ui.deviceTreeWidget->selectedItems();

	bool first = true;
	for (QTreeWidgetItem* item : list)
	{
		if (item->childCount() != 0)
			continue;

		if (first)
			first = false;
		else
			command += "; ";

		std::shared_ptr<AbstractAPOInfo> info = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
		command += QString::fromStdWString(info->getDeviceString()).replace(';', ' ');
	}

	QClipboard* clipboard = QGuiApplication::clipboard();
	clipboard->setText(command);
}

void DeviceSelector::onTroubleShootingToggled(bool on)
{
	if (on)
		ui.troubleshootingGroupBox->setStyleSheet("");
	else
		ui.troubleshootingGroupBox->setStyleSheet("#" + ui.troubleshootingGroupBox->objectName() + " {border:0;}");

	ui.stackedWidget->setVisible(on);
}

void DeviceSelector::onTroubleShootingOptionChanged()
{
	QList<QTreeWidgetItem*> list = ui.deviceTreeWidget->selectedItems();
	for (QTreeWidgetItem* item : list)
	{
		if (item->childCount() != 0)
			continue;

		std::shared_ptr<AbstractAPOInfo> info = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
		DeviceAPOInfo* deviceInfo = dynamic_cast<DeviceAPOInfo*>(info.get());
		if (deviceInfo != NULL)
		{
			QObject* sender = QObject::sender();
			if (sender == ui.installPreMixCheckBox)
				deviceInfo->getSelectedInstallState().installPreMix = ui.installPreMixCheckBox->isChecked();
			else if (sender == ui.installPostMixCheckBox)
				deviceInfo->getSelectedInstallState().installPostMix = ui.installPostMixCheckBox->isChecked();
			else if (sender == ui.useOriginalAPOPreMixCheckBox)
				deviceInfo->getSelectedInstallState().useOriginalAPOPreMix = ui.useOriginalAPOPreMixCheckBox->isChecked();
			else if (sender == ui.useOriginalAPOPostMixCheckBox)
				deviceInfo->getSelectedInstallState().useOriginalAPOPostMix = ui.useOriginalAPOPostMixCheckBox->isChecked();
			else if (sender == ui.installModeComboBox)
				deviceInfo->getSelectedInstallState().installMode = (DeviceAPOInfo::InstallMode)ui.installModeComboBox->currentIndex();
			else if (sender == ui.allowSilentBufferCheckBox)
				deviceInfo->getSelectedInstallState().allowSilentBufferModification = ui.allowSilentBufferCheckBox->isChecked();
			else if (sender == ui.autoCheckBox)
				deviceInfo->getSelectedInstallState().autoAdjust = ui.autoCheckBox->isChecked();
		}

		updateList(item);
	}

	updateButtons();
}

void DeviceSelector::updateList(QTreeWidgetItem* item)
{
	std::shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
	bool checked = item->checkState(0) == Qt::Checked;

	QString state = getStateText(apoInfo, checked);
	item->setText(2, state);

	ui.deviceTreeWidget->resizeColumnToContents(2);
}

void DeviceSelector::updateButtons()
{
	bool changed = isChanged();
	if (changed || !isAnySelected())
	{
		QPushButton* okButton = ui.buttonBox->button(QDialogButtonBox::Ok);
		okButton->setVisible(true);
		okButton->setEnabled(changed);
		QPushButton* cancelButton = ui.buttonBox->button(QDialogButtonBox::Cancel);
		cancelButton->setText(tr("Cancel"));
	}
	else
	{
		QPushButton* okButton = ui.buttonBox->button(QDialogButtonBox::Ok);
		okButton->setVisible(false);
		QPushButton* cancelButton = ui.buttonBox->button(QDialogButtonBox::Cancel);
		cancelButton->setText(tr("Close"));
	}

	QList<QTreeWidgetItem*> list = ui.deviceTreeWidget->selectedItems();
	bool noGroupsSelected = !list.isEmpty();
	for (QTreeWidgetItem* item : list)
	{
		if (item->childCount() != 0)
		{
			noGroupsSelected = false;
			break;
		}
	}

	ui.copyDeviceCommandAction->setEnabled(noGroupsSelected);

	bool enable = false;
	bool isInput = false;
	bool hasOriginalAPOPreMix = true;
	bool hasOriginalAPOPostMix = true;
	DeviceAPOInfo::InstallState installState;
	if (noGroupsSelected && list.size() == 1)
	{
		QTreeWidgetItem* item = list[0];
		enable = item->checkState(0) == Qt::Checked;

		std::shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
		DeviceAPOInfo* deviceApoInfo = dynamic_cast<DeviceAPOInfo*>(apoInfo.get());
		if (deviceApoInfo != NULL)
		{
			isInput = deviceApoInfo->isInput();
			hasOriginalAPOPreMix = deviceApoInfo->getOriginalAPOPreMix() != L"";
			hasOriginalAPOPostMix = deviceApoInfo->getOriginalAPOPostMix() != L"";
			installState = deviceApoInfo->getSelectedInstallState();
		}
	}

	ui.preMixLabel->setEnabled(enable);
	ui.postMixLabel->setEnabled(enable && !isInput);
	ui.installPreMixCheckBox->setEnabled(enable);
	ui.installPostMixCheckBox->setEnabled(enable && !isInput);
	ui.useOriginalAPOPreMixCheckBox->setEnabled(enable && hasOriginalAPOPreMix && installState.installPreMix);
	ui.useOriginalAPOPostMixCheckBox->setEnabled(enable && !isInput && hasOriginalAPOPostMix && installState.installPostMix);
	ui.installModeComboBox->setEnabled(enable);
	ui.allowSilentBufferCheckBox->setEnabled(enable);
	ui.stackedWidget->setCurrentIndex(enable ? 1 : 0);

	ui.installPreMixCheckBox->setChecked(installState.installPreMix);
	ui.installPostMixCheckBox->setChecked(installState.installPostMix);
	ui.useOriginalAPOPreMixCheckBox->setChecked(installState.useOriginalAPOPreMix && hasOriginalAPOPreMix);
	ui.useOriginalAPOPostMixCheckBox->setChecked(installState.useOriginalAPOPostMix && hasOriginalAPOPostMix);

	if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		ui.installModeComboBox->setCurrentIndex(installState.installMode);

	ui.allowSilentBufferCheckBox->setChecked(installState.allowSilentBufferModification);
	ui.autoCheckBox->setChecked(installState.autoAdjust);
}

bool DeviceSelector::isAnySelected()
{
	bool anySelected = false;

	for (int index = 0; index < ui.deviceTreeWidget->topLevelItemCount(); index++)
	{
		QTreeWidgetItem* topItem = ui.deviceTreeWidget->topLevelItem(index);
		for (int i = 0; i < topItem->childCount(); i++)
		{
			QTreeWidgetItem* item = topItem->child(i);
			if (item->checkState(0) == Qt::Checked)
			{
				anySelected = true;
				break;
			}
		}
	}

	return anySelected;
}

bool DeviceSelector::isChanged()
{
	bool changed = false;

	for (int index = 0; index < ui.deviceTreeWidget->topLevelItemCount(); index++)
	{
		QTreeWidgetItem* topItem = ui.deviceTreeWidget->topLevelItem(index);
		for (int i = 0; i < topItem->childCount(); i++)
		{
			QTreeWidgetItem* item = topItem->child(i);
			std::shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
			bool checked = item->checkState(0) == Qt::Checked;
			if (checked != apoInfo->isInstalled()
				|| checked && apoInfo->isInstalled() && (apoInfo->canBeUpgraded() || apoInfo->hasChanges() || apoInfo->isEnhancementsDisabled()))
			{
				changed = true;
				break;
			}
		}
	}

	return changed;
}

bool DeviceSelector::hasUpgrades()
{
	bool hasUpgrades = false;

	for (int index = 0; index < ui.deviceTreeWidget->topLevelItemCount(); index++)
	{
		QTreeWidgetItem* topItem = ui.deviceTreeWidget->topLevelItem(index);
		for (int i = 0; i < topItem->childCount(); i++)
		{
			QTreeWidgetItem* item = topItem->child(i);
			std::shared_ptr<AbstractAPOInfo> apoInfo = item->data(0, Qt::UserRole).value<std::shared_ptr<AbstractAPOInfo>>();
			bool checked = item->checkState(0) == Qt::Checked;
			if (checked && apoInfo->isInstalled() && (apoInfo->canBeUpgraded() || apoInfo->isEnhancementsDisabled()))
			{
				hasUpgrades = true;
				break;
			}
		}
	}

	return hasUpgrades;
}

QString DeviceSelector::getStateText(const std::shared_ptr<AbstractAPOInfo>& apoInfo, bool checked)
{
	QString state;
	if (checked && !apoInfo->isInstalled())
		state = tr("APO will be installed");
	else if (!checked && apoInfo->isInstalled())
		state = tr("APO will be uninstalled");
	else if (apoInfo->isInstalled() && apoInfo->canBeUpgraded())
		state = tr("APO will be upgraded");
	else if (apoInfo->isInstalled() && apoInfo->hasChanges())
		state = tr("APO installation will be changed");
	else if (apoInfo->isInstalled() && apoInfo->isEnhancementsDisabled())
		state = tr("Audio enhancements will be enabled");
	else if (apoInfo->isInstalled())
		state = tr("APO is already installed");
	else if (apoInfo->isExperimental())
		state = tr("APO can be installed (experimental)");
	else
		state = tr("APO can be installed");

	VoicemeeterAPOInfo* voicemeeterInfo = dynamic_cast<VoicemeeterAPOInfo*>(apoInfo.get());
	if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
		state += ", " + tr("Voicemeeter was uninstalled");
	else if (apoInfo->isDefaultDevice())
		state += ", " + tr("Default device");

	if (apoInfo->isDisabled())
		state += ", " + tr("Disabled");
	if (apoInfo->isUnplugged())
		state += ", " + tr("Unplugged");

	return state;
}
