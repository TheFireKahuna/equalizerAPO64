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

#pragma once

#include "resource.h"
#include <string>
#include <vector>
#include <memory>
#include "DeviceAPOInfo.h"

class Configurator
{
public:
	Configurator(HINSTANCE hInstance, const wchar_t* cmdLine);
	void onInitDialog(HWND hDlg);
	void onLvnItemChanged(unsigned sourceId, LPNMLISTVIEW info);
	bool onButtonClicked(unsigned sourceId);
	void onTcnSelChange(unsigned sourceId);
	bool isAnySelected();
	bool isChanged();
	bool hasUpgrades();

private:
	HINSTANCE hInstance;
	HWND hDlg;
	HWND categoryTabCtrl;
	HWND deviceLists[2];
	HWND okButton;
	HWND cancelButton;
	HWND copyDeviceCommandButton;
	HWND requestLabel;
	HWND toggleTroubleShooting;
	HWND troubleShootingGroup;
	HWND preMixLabel;
	HWND postMixLabel;
	HWND installPreMix;
	HWND installPostMix;
	HWND useOriginalAPOPreMix;
	HWND useOriginalAPOPostMix;
	HWND installModeComboBox;
	HWND allowSilentBuffer;
	HWND selectOneDeviceLabel;

	std::wstring cmdLine;
	void expandTroubleShooting(bool expand);
	HDWP moveWindow(HDWP hdwp, HWND hWnd, int x, int y);
	void updateList(int listIndex, int itemIndex);
	void updateButtons(int listIndex);

	std::vector<std::shared_ptr<AbstractAPOInfo> > apoInfos[2];
	bool askForReboot = false;
};
