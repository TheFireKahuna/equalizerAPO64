/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2012  Jonas Thedering

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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <ObjBase.h>
#include "helpers/StringHelper.h"
#include "helpers/RegistryHelper.h"
#include "VoicemeeterAPOInfo.h"
#include "Configurator.h"

using namespace std;

Configurator* configurator;

Configurator::Configurator(HINSTANCE hInstance, const wchar_t* cmdLine)
	: cmdLine(cmdLine)
{
	this->hInstance = hInstance;
	hDlg = NULL;
}

void Configurator::onInitDialog(HWND hDlg)
{
	this->hDlg = hDlg;
	categoryTabCtrl = GetDlgItem(hDlg, IDC_CATEGORY_TAB_CTRL);
	deviceLists[0] = GetDlgItem(hDlg, IDC_PLAYBACK_LIST);
	deviceLists[1] = GetDlgItem(hDlg, IDC_CAPTURE_LIST);
	okButton = GetDlgItem(hDlg, IDOK);
	cancelButton = GetDlgItem(hDlg, IDCANCEL);
	requestLabel = GetDlgItem(hDlg, IDC_REQUEST);
	copyDeviceCommandButton = GetDlgItem(hDlg, IDC_COPY_DEVICE_COMMAND);
	toggleTroubleShooting = GetDlgItem(hDlg, IDC_TOGGLE_TROUBLESHOOTING);
	troubleShootingGroup = GetDlgItem(hDlg, IDC_TROUBLESHOOTING_GROUP);
	preMixLabel = GetDlgItem(hDlg, IDC_PRE_MIX_LABEL);
	postMixLabel = GetDlgItem(hDlg, IDC_POST_MIX_LABEL);
	installPreMix = GetDlgItem(hDlg, IDC_INSTALL_PRE_MIX);
	installPostMix = GetDlgItem(hDlg, IDC_INSTALL_POST_MIX);
	useOriginalAPOPreMix = GetDlgItem(hDlg, IDC_USE_ORIGINAL_APO_PRE_MIX);
	useOriginalAPOPostMix = GetDlgItem(hDlg, IDC_USE_ORIGINAL_APO_POST_MIX);
	installModeComboBox = GetDlgItem(hDlg, IDC_INSTALL_MODE_COMBOBOX);
	allowSilentBuffer = GetDlgItem(hDlg, IDC_ALLOW_SILENT_BUFFER);
	selectOneDeviceLabel = GetDlgItem(hDlg, IDC_SELECT_ONE_DEVICE);

	expandTroubleShooting(false);

	wchar_t stringBuf[512];

	TCITEM tci;
	tci.pszText = stringBuf;
	tci.mask = TCIF_TEXT;

	LoadStringW(hInstance, IDS_PLAYBACK_DEVICES, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	TabCtrl_InsertItem(categoryTabCtrl, 0, &tci);

	LoadStringW(hInstance, IDS_CAPTURE_DEVICES, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	TabCtrl_InsertItem(categoryTabCtrl, 1, &tci);

	LoadStringW(hInstance, IDS_REQUEST, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(requestLabel, stringBuf);

	LoadStringW(hInstance, IDS_COPY_DEVICE_COMMAND, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(copyDeviceCommandButton, stringBuf);

	LoadStringW(hInstance, IDS_OK, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(okButton, stringBuf);

	LoadStringW(hInstance, IDS_CANCEL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(cancelButton, stringBuf);

	LoadStringW(hInstance, IDS_TOGGLE_TROUBLESHOOTING, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(toggleTroubleShooting, stringBuf);
	int width = 185;
	if (wcsncmp(stringBuf, L"Probleml", 8) == 0)
		width = 215;
	RECT rect = {0, 0, width, 10};
	MapDialogRect(hDlg, &rect);
	SetWindowPos(toggleTroubleShooting, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

	LoadStringW(hInstance, IDS_PRE_MIX_LABEL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(preMixLabel, stringBuf);

	LoadStringW(hInstance, IDS_POST_MIX_LABEL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(postMixLabel, stringBuf);

	LoadStringW(hInstance, IDS_INSTALL_APO, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(installPreMix, stringBuf);
	SetWindowTextW(installPostMix, stringBuf);

	LoadStringW(hInstance, IDS_USE_ORIGINAL_APO, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(useOriginalAPOPreMix, stringBuf);
	SetWindowTextW(useOriginalAPOPostMix, stringBuf);

	LoadStringW(hInstance, IDS_INSTALL_MODE_LFX_GFX, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	ComboBox_AddString(installModeComboBox, stringBuf);

	if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
	{
		LoadStringW(hInstance, IDS_INSTALL_MODE_SFX_MFX, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		ComboBox_AddString(installModeComboBox, stringBuf);

		LoadStringW(hInstance, IDS_INSTALL_MODE_SFX_EFX, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		ComboBox_AddString(installModeComboBox, stringBuf);
	}
	ComboBox_SetCurSel(installModeComboBox, 0);

	LoadStringW(hInstance, IDS_ALLOW_SILENT_BUFFER, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(allowSilentBuffer, stringBuf);

	LoadStringW(hInstance, IDS_ALLOW_SILENT_BUFFER_TOOLTIP, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	HWND tooltip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			hDlg, NULL,
			hInstance, NULL);

	if (tooltip != NULL)
	{
		TOOLINFO toolInfo = {0};
		toolInfo.cbSize = sizeof(toolInfo);
		toolInfo.hwnd = hDlg;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR)allowSilentBuffer;
		toolInfo.lpszText = stringBuf;
		SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		SendMessage(tooltip, TTM_SETMAXTIPWIDTH, 0, 400);
	}

	LoadStringW(hInstance, IDS_SELECT_ONE_DEVICE, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
	SetWindowTextW(selectOneDeviceLabel, stringBuf);

	for (int i = 0; i <= 1; i++)
	{
		HWND deviceList = deviceLists[i];
		ListView_SetExtendedListViewStyle(deviceList, ListView_GetExtendedListViewStyle(deviceList) | LVS_EX_CHECKBOXES);
		LVCOLUMN column;
		column.pszText = stringBuf;
		column.mask = LVCF_TEXT;

		LoadStringW(hInstance, IDS_CONNECTOR, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		ListView_InsertColumn(deviceList, 0, &column);

		LoadStringW(hInstance, IDS_DEVICE, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		ListView_InsertColumn(deviceList, 1, &column);

		LoadStringW(hInstance, IDS_STATUS, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		ListView_InsertColumn(deviceList, 2, &column);

		LVITEM item;
		item.iSubItem = 0;
		item.pszText = stringBuf;
		item.mask = LVIF_TEXT | LVIF_PARAM;

		try
		{
			apoInfos[i] = DeviceAPOInfo::loadAllInfos(i == 1);
			wstring defaultDevice = StringHelper::toLowerCase(DeviceAPOInfo::getDefaultDevice(i == 1));

			int itemCount = 0;
			for (shared_ptr<AbstractAPOInfo>& apoInfo : apoInfos[i])
			{
				wcsncpy_s(stringBuf, sizeof(stringBuf) / sizeof(wchar_t), apoInfo->getConnectionName().c_str(), _TRUNCATE);
				item.iItem = itemCount;
				item.lParam = itemCount;
				ListView_InsertItem(deviceList, &item);
				wcsncpy_s(stringBuf, sizeof(stringBuf) / sizeof(wchar_t), apoInfo->getDeviceName().c_str(), _TRUNCATE);
				ListView_SetItemText(deviceList, itemCount, 1, stringBuf);

				VoicemeeterAPOInfo* voicemeeterInfo = dynamic_cast<VoicemeeterAPOInfo*>(apoInfo.get());
				if (apoInfo->isInstalled())
				{
					if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
					{
						ListView_SetCheckState(deviceList, itemCount, FALSE);
						LoadStringW(hInstance, IDS_WILL_BE_UNINSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
					}
					else
					{
						ListView_SetCheckState(deviceList, itemCount, TRUE);
						if (apoInfo->canBeUpgraded())
							LoadStringW(hInstance, IDS_WILL_BE_UPGRADED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
						else if (apoInfo->hasChanges())
							LoadStringW(hInstance, IDS_WILL_BE_CHANGED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
						else if (apoInfo->isEnhancementsDisabled())
							LoadStringW(hInstance, IDS_ENHANCEMENTS_WILL_BE_ENABLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
						else
							LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
					}
				}
				else if (apoInfo->isExperimental())
				{
					LoadStringW(hInstance, IDS_CAN_BE_INSTALLED_EXPERIMENTAL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
				}
				else
				{
					LoadStringW(hInstance, IDS_CAN_BE_INSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
				}

				if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
				{
					wstring statusText = stringBuf;
					LoadStringW(hInstance, IDS_VOICEMEETER_WAS_UNINSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
					statusText = wstring(stringBuf) + L", " + statusText;

					ListView_SetItemText(deviceList, itemCount, 2, const_cast<wchar_t*>(statusText.c_str()));
				}
				else if (apoInfo->isDefaultDevice())
				{
					wstring statusText = stringBuf;
					LoadStringW(hInstance, IDS_DEFAULT_DEVICE, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
					statusText = wstring(stringBuf) + L", " + statusText;

					ListView_SetItemText(deviceList, itemCount, 2, const_cast<wchar_t*>(statusText.c_str()));
				}
				else
				{
					ListView_SetItemText(deviceList, itemCount, 2, stringBuf);
				}

				itemCount++;
			}
		}
		catch (RegistryException e)
		{
			MessageBoxW(hDlg, e.getMessage().c_str(), L"Error while accessing the registry", MB_ICONERROR | MB_OK);
		}

		ListView_SetColumnWidth(deviceList, 0, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(deviceList, 1, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(deviceList, 2, LVSCW_AUTOSIZE);
	}

	bool fixedAudioDG = !DeviceAPOInfo::checkProtectedAudioDG(true);
	bool fixedRegistration = !DeviceAPOInfo::checkAPORegistration(true);
	if (fixedAudioDG || fixedRegistration)
	{
		LoadStringW(hInstance, IDS_REGISTRY_VALUE_FIXED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		MessageBoxW(hDlg, stringBuf, L"Info", MB_ICONINFORMATION | MB_OK);
		askForReboot = true;
	}
}

void Configurator::onLvnItemChanged(unsigned sourceId, LPNMLISTVIEW info)
{
	int index = (sourceId == IDC_PLAYBACK_LIST ? 0 : 1);

	updateList(index, info->iItem);
	updateButtons(index);
}

bool Configurator::onButtonClicked(unsigned sourceId)
{
	bool deviceUpdated = false;

	switch (sourceId)
	{
	case IDOK:
		{
			for (int index = 0; index <= 1; index++)
			{
				HWND deviceList = deviceLists[index];

				for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
				{
					LVITEM item;
					item.iItem = i;
					item.iSubItem = 0;
					item.mask = LVIF_PARAM;
					ListView_GetItem(deviceList, &item);

					try
					{
						shared_ptr<AbstractAPOInfo>& info = apoInfos[index][item.lParam];
						DeviceAPOInfo* deviceInfo = dynamic_cast<DeviceAPOInfo*>(info.get());
						if (ListView_GetCheckState(deviceList, i) && !info->isInstalled())
						{
							info->install();
							if (deviceInfo != NULL)
								deviceUpdated = true;
						}
						else if (!ListView_GetCheckState(deviceList, i) && info->isInstalled())
						{
							info->uninstall();
							if (deviceInfo != NULL)
								deviceUpdated = true;
						}
						else if (ListView_GetCheckState(deviceList, i) && (info->canBeUpgraded() || info->hasChanges() || info->isEnhancementsDisabled()))
						{
							info->reinstall();
							if (deviceInfo != NULL)
								deviceUpdated = true;
						}
					}
					catch (RegistryException e)
					{
						MessageBoxW(hDlg, e.getMessage().c_str(), L"Error while accessing the registry", MB_ICONERROR | MB_OK);
					}
				}
			}

			VoicemeeterAPOInfo::ensureVoicemeeterClientRunning();
		}
		break;
	case IDC_COPY_DEVICE_COMMAND:
		{
			wstring command = L"Device: ";

			int index = TabCtrl_GetCurSel(categoryTabCtrl);
			HWND deviceList = deviceLists[index];

			bool first = true;
			for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
			{
				LVITEM item;
				item.iItem = i;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM | LVIF_STATE;
				item.stateMask = LVIS_SELECTED;
				ListView_GetItem(deviceList, &item);

				if (item.state & LVIS_SELECTED)
				{
					if (first)
						first = false;
					else
						command += L"; ";

					shared_ptr<AbstractAPOInfo>& info = apoInfos[index][item.lParam];
					command += StringHelper::replaceCharacters(info->getDeviceString(), L";", L" ");
				}
			}

			OpenClipboard(hDlg);

			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (command.length() + 1) * sizeof(wchar_t));
			wchar_t* data = (wchar_t*)GlobalLock(hData);
			memcpy(data, command.c_str(), (command.length() + 1) * sizeof(wchar_t));
			GlobalUnlock(data);

			SetClipboardData(CF_UNICODETEXT, hData);

			CloseClipboard();
		}
		break;
	case IDC_TOGGLE_TROUBLESHOOTING:
		{
			bool checked = IsDlgButtonChecked(hDlg, IDC_TOGGLE_TROUBLESHOOTING) == BST_CHECKED;
			expandTroubleShooting(checked);
		}
		break;
	case IDC_INSTALL_PRE_MIX:
	case IDC_INSTALL_POST_MIX:
	case IDC_USE_ORIGINAL_APO_PRE_MIX:
	case IDC_USE_ORIGINAL_APO_POST_MIX:
	case IDC_INSTALL_MODE_COMBOBOX:
	case IDC_ALLOW_SILENT_BUFFER:
		{
			int index = TabCtrl_GetCurSel(categoryTabCtrl);
			HWND deviceList = deviceLists[index];

			unsigned selectedCount = ListView_GetSelectedCount(deviceList);
			for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
			{
				LVITEM item;
				item.iItem = i;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM | LVIF_STATE;
				item.stateMask = LVIS_SELECTED;
				ListView_GetItem(deviceList, &item);

				if (item.state & LVIS_SELECTED)
				{
					shared_ptr<AbstractAPOInfo>& info = apoInfos[index][item.lParam];
					DeviceAPOInfo* deviceInfo = dynamic_cast<DeviceAPOInfo*>(info.get());
					if (deviceInfo != NULL)
					{
						switch (sourceId)
						{
						case IDC_INSTALL_PRE_MIX:
							deviceInfo->getSelectedInstallState().installPreMix = Button_GetCheck(installPreMix) == BST_CHECKED;
							break;
						case IDC_INSTALL_POST_MIX:
							deviceInfo->getSelectedInstallState().installPostMix = Button_GetCheck(installPostMix) == BST_CHECKED;
							break;
						case IDC_USE_ORIGINAL_APO_PRE_MIX:
							deviceInfo->getSelectedInstallState().useOriginalAPOPreMix = Button_GetCheck(useOriginalAPOPreMix) == BST_CHECKED;
							break;
						case IDC_USE_ORIGINAL_APO_POST_MIX:
							deviceInfo->getSelectedInstallState().useOriginalAPOPostMix = Button_GetCheck(useOriginalAPOPostMix) == BST_CHECKED;
							break;
						case IDC_INSTALL_MODE_COMBOBOX:
							deviceInfo->getSelectedInstallState().installMode = (DeviceAPOInfo::InstallMode)ComboBox_GetCurSel(installModeComboBox);
							break;
						case IDC_ALLOW_SILENT_BUFFER:
							deviceInfo->getSelectedInstallState().allowSilentBufferModification = Button_GetCheck(allowSilentBuffer) == BST_CHECKED;
							break;
						}

						updateList(index, i);
					}
				}
			}

			updateButtons(index);
		}
		break;
	}

	if (sourceId == IDCANCEL && hasUpgrades())
	{
		wchar_t captionBuf[255];
		LoadStringW(hInstance, IDS_CANCEL_UPGRADES_CAPTION, captionBuf, sizeof(captionBuf) / sizeof(wchar_t));
		wchar_t stringBuf[255];
		LoadStringW(hInstance, IDS_CANCEL_UPGRADES, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		if (MessageBoxW(hDlg, stringBuf, captionBuf, MB_ICONWARNING | MB_YESNO) == IDNO)
			return false;
	}

	if (sourceId == IDOK || sourceId == IDCANCEL)
	{
		if (cmdLine == L"/i")
		{
			wchar_t stringBuf[255];
			LoadStringW(hInstance, IDS_AFTERINSTALL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
			MessageBoxW(hDlg, stringBuf, L"Info", MB_ICONINFORMATION | MB_OK);
		}
		else if (sourceId == IDOK && deviceUpdated && RegistryHelper::isWindowsVersionAtLeast(6, 3) // Windows 8.1
			|| askForReboot)
		{
			wchar_t captionBuf[255];
			LoadStringW(hInstance, IDS_SHOULD_REBOOT_CAPTION, captionBuf, sizeof(captionBuf) / sizeof(wchar_t));
			wchar_t stringBuf[255];
			LoadStringW(hInstance, IDS_SHOULD_REBOOT, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
			if (MessageBoxW(hDlg, stringBuf, captionBuf, MB_ICONQUESTION | MB_YESNO) == IDYES)
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

		return true;
	}

	return false;
}

void Configurator::onTcnSelChange(unsigned sourceId)
{
	int index = TabCtrl_GetCurSel(categoryTabCtrl);

	ShowWindow(deviceLists[index], SW_SHOW);
	ShowWindow(deviceLists[1 - index], SW_HIDE);

	SetFocus(deviceLists[index]);

	updateButtons(index);
}

bool Configurator::isAnySelected()
{
	bool anySelected = false;

	for (int index = 0; index <= 1; index++)
	{
		HWND deviceList = deviceLists[index];

		for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			if (ListView_GetCheckState(deviceList, i) != 0)
			{
				anySelected = true;
				break;
			}
		}
	}

	return anySelected;
}

bool Configurator::isChanged()
{
	bool changed = false;

	for (int index = 0; index <= 1; index++)
	{
		HWND deviceList = deviceLists[index];

		for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			shared_ptr<AbstractAPOInfo>& apoInfo = apoInfos[index][item.lParam];
			if ((ListView_GetCheckState(deviceList, i) != 0) != apoInfo->isInstalled()
				|| ListView_GetCheckState(deviceList, i) && apoInfo->isInstalled() && (apoInfo->canBeUpgraded() || apoInfo->hasChanges() || apoInfo->isEnhancementsDisabled()))
			{
				changed = true;
				break;
			}
		}
	}

	return changed;
}

bool Configurator::hasUpgrades()
{
	bool hasUpgrades = false;

	for (int index = 0; index <= 1; index++)
	{
		HWND deviceList = deviceLists[index];

		for (int i = 0; i < ListView_GetItemCount(deviceList); i++)
		{
			LVITEM item;
			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_PARAM;
			ListView_GetItem(deviceList, &item);
			shared_ptr<AbstractAPOInfo>& apoInfo = apoInfos[index][item.lParam];
			if (ListView_GetCheckState(deviceList, i) && apoInfo->isInstalled() && (apoInfo->canBeUpgraded() || apoInfo->isEnhancementsDisabled()))
			{
				hasUpgrades = true;
				break;
			}
		}
	}

	return hasUpgrades;
}

void Configurator::expandTroubleShooting(bool expand)
{
	int showCmd = expand ? SW_SHOW : SW_HIDE;
	ShowWindow(troubleShootingGroup, showCmd);
	showCmd = expand && !IsWindowEnabled(selectOneDeviceLabel) ? SW_SHOW : SW_HIDE;
	ShowWindow(preMixLabel, showCmd);
	ShowWindow(postMixLabel, showCmd);
	ShowWindow(postMixLabel, showCmd);
	ShowWindow(installPreMix, showCmd);
	ShowWindow(installPostMix, showCmd);
	ShowWindow(useOriginalAPOPreMix, showCmd);
	ShowWindow(useOriginalAPOPostMix, showCmd);
	ShowWindow(installModeComboBox, showCmd);
	ShowWindow(allowSilentBuffer, showCmd);
	showCmd = expand && IsWindowEnabled(selectOneDeviceLabel) ? SW_SHOW : SW_HIDE;
	ShowWindow(selectOneDeviceLabel, showCmd);

	int y = expand ? 229 : 188;

	HDWP hdwp = BeginDeferWindowPos(3);
	hdwp = moveWindow(hdwp, copyDeviceCommandButton, 7, y);
	hdwp = moveWindow(hdwp, okButton, 285, y);
	hdwp = moveWindow(hdwp, cancelButton, 343, y);
	EndDeferWindowPos(hdwp);

	WINDOWINFO windowInfo;
	GetWindowInfo(hDlg, &windowInfo);
	int height = expand ? 250 : 209;
	RECT rect = {0, 0, 401, height};
	MapDialogRect(hDlg, &rect);
	AdjustWindowRect(&rect, windowInfo.dwStyle, false);
	SetWindowPos(hDlg, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
}

HDWP Configurator::moveWindow(HDWP hdwp, HWND hWnd, int x, int y)
{
	RECT rect = {x, y, 0, 0};
	MapDialogRect(hDlg, &rect);
	return DeferWindowPos(hdwp, hWnd, NULL, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

void Configurator::updateList(int listIndex, int itemIndex)
{
	HWND deviceList = deviceLists[listIndex];

	int itemCount = ListView_GetItemCount(deviceList);

	if (apoInfos[listIndex].size() == itemCount)
	{
		shared_ptr<AbstractAPOInfo>& apoInfo = apoInfos[listIndex][itemIndex];
		bool checked = ListView_GetCheckState(deviceList, itemIndex) != 0;
		wchar_t stringBuf[255];
		if (checked && !apoInfo->isInstalled())
			LoadStringW(hInstance, IDS_WILL_BE_INSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (!checked && apoInfo->isInstalled())
			LoadStringW(hInstance, IDS_WILL_BE_UNINSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (apoInfo->isInstalled() && apoInfo->canBeUpgraded())
			LoadStringW(hInstance, IDS_WILL_BE_UPGRADED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (apoInfo->isInstalled() && apoInfo->hasChanges())
			LoadStringW(hInstance, IDS_WILL_BE_CHANGED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (apoInfo->isInstalled() && apoInfo->isEnhancementsDisabled())
			LoadStringW(hInstance, IDS_ENHANCEMENTS_WILL_BE_ENABLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (apoInfo->isInstalled())
			LoadStringW(hInstance, IDS_ALREADY_INSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else if (apoInfo->isExperimental())
			LoadStringW(hInstance, IDS_CAN_BE_INSTALLED_EXPERIMENTAL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		else
			LoadStringW(hInstance, IDS_CAN_BE_INSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));

		VoicemeeterAPOInfo* voicemeeterInfo = dynamic_cast<VoicemeeterAPOInfo*>(apoInfo.get());
		if (voicemeeterInfo != NULL && !voicemeeterInfo->isVoicemeeterInstalled())
		{
			wstring statusText = stringBuf;
			LoadStringW(hInstance, IDS_VOICEMEETER_WAS_UNINSTALLED, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
			statusText = wstring(stringBuf) + L", " + statusText;

			ListView_SetItemText(deviceList, itemCount, 2, const_cast<wchar_t*>(statusText.c_str()));
		}
		else if (apoInfo->isDefaultDevice())
		{
			wstring statusText = stringBuf;
			LoadStringW(hInstance, IDS_DEFAULT_DEVICE, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
			statusText = wstring(stringBuf) + L", " + statusText;

			ListView_SetItemText(deviceList, itemIndex, 2, const_cast<wchar_t*>(statusText.c_str()));
		}
		else
		{
			ListView_SetItemText(deviceList, itemIndex, 2, stringBuf);
		}

		ListView_SetColumnWidth(deviceList, 2, LVSCW_AUTOSIZE);
	}
}

void Configurator::updateButtons(int listIndex)
{
	HWND deviceList = deviceLists[listIndex];

	wchar_t stringBuf[255];
	bool changed = isChanged();
	if (changed || !isAnySelected())
	{
		ShowWindow(okButton, SW_SHOW);
		EnableWindow(okButton, changed);
		LoadStringW(hInstance, IDS_CANCEL, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		SetWindowText(cancelButton, stringBuf);
	}
	else
	{
		ShowWindow(okButton, SW_HIDE);
		LoadStringW(hInstance, IDS_CLOSE, stringBuf, sizeof(stringBuf) / sizeof(wchar_t));
		SetWindowText(cancelButton, stringBuf);
	}

	unsigned selectedCount = ListView_GetSelectedCount(deviceList);
	EnableWindow(copyDeviceCommandButton, selectedCount > 0);

	bool enable = false;
	bool isInput = false;
	bool hasOriginalAPOPreMix = true;
	bool hasOriginalAPOPostMix = true;
	DeviceAPOInfo::InstallState installState;
	if (selectedCount == 1)
	{
		int selectedIndex = ListView_GetNextItem(deviceList, -1, LVNI_SELECTED);
		if (selectedIndex != -1)
		{
			enable = ListView_GetCheckState(deviceList, selectedIndex) != 0;

			shared_ptr<AbstractAPOInfo>& apoInfo = apoInfos[listIndex][selectedIndex];
			DeviceAPOInfo* deviceApoInfo = dynamic_cast<DeviceAPOInfo*>(apoInfo.get());
			if (deviceApoInfo != NULL)
			{
				isInput = deviceApoInfo->isInput();
				hasOriginalAPOPreMix = deviceApoInfo->getOriginalAPOPreMix() != L"";
				hasOriginalAPOPostMix = deviceApoInfo->getOriginalAPOPostMix() != L"";
				installState = deviceApoInfo->getSelectedInstallState();
			}
		}
	}

	EnableWindow(preMixLabel, enable);
	EnableWindow(postMixLabel, enable && !isInput);
	EnableWindow(installPreMix, enable);
	EnableWindow(installPostMix, enable && !isInput);
	EnableWindow(useOriginalAPOPreMix, enable && hasOriginalAPOPreMix && installState.installPreMix);
	EnableWindow(useOriginalAPOPostMix, enable && !isInput && hasOriginalAPOPostMix && installState.installPostMix);
	EnableWindow(installModeComboBox, enable);
	EnableWindow(allowSilentBuffer, enable);
	EnableWindow(selectOneDeviceLabel, !enable);

	Button_SetCheck(installPreMix, installState.installPreMix);
	Button_SetCheck(installPostMix, installState.installPostMix);
	Button_SetCheck(useOriginalAPOPreMix, installState.useOriginalAPOPreMix && hasOriginalAPOPreMix);
	Button_SetCheck(useOriginalAPOPostMix, installState.useOriginalAPOPostMix && hasOriginalAPOPostMix);

	if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		ComboBox_SetCurSel(installModeComboBox, installState.installMode);

	Button_SetCheck(allowSilentBuffer, installState.allowSilentBufferModification);

	expandTroubleShooting(Button_GetCheck(toggleTroubleShooting) != 0);
}

INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(HINSTANCE hInstance,
	HINSTANCE /* hPrevInstance */,
	LPWSTR lpCmdLine,
	int nCmdShow)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	int result = 0;

	if (wcscmp(lpCmdLine, L"/u") == 0)
	{
		for (int index = 0; index <= 1; index++)
		{
			vector<shared_ptr<AbstractAPOInfo> > apoInfos = DeviceAPOInfo::loadAllInfos(index == 1);

			for (shared_ptr<AbstractAPOInfo>& apoInfo : apoInfos)
			{
				try
				{
					if (apoInfo->isInstalled())
						apoInfo->uninstall();
				}
				catch (RegistryException e)
				{
					MessageBoxW(NULL, e.getMessage().c_str(), NULL, MB_ICONERROR | MB_OK);
					result = -1;
				}
			}
		}

		VoicemeeterAPOInfo::ensureVoicemeeterClientRunning();
	}
	else
	{
		configurator = new Configurator(hInstance, lpCmdLine);
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINWINDOW), GetDesktopWindow(), dlgProc);
	}

	CoUninitialize();

	return result;
}

INT_PTR CALLBACK dlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		configurator->onInitDialog(hDlg);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (configurator->onButtonClicked(LOWORD(wParam)))
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case LVN_ITEMCHANGED:
			configurator->onLvnItemChanged((unsigned)((LPNMHDR)lParam)->idFrom, (LPNMLISTVIEW) lParam);
			break;
		case TCN_SELCHANGE:
			configurator->onTcnSelChange((unsigned)((LPNMHDR)lParam)->idFrom);
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}