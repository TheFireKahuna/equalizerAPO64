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

#include <string>
#include <vector>
#include <memory>
#include "AbstractAPOInfo.h"

#define APOGUID_NULL L"{00000000-0000-0000-0000-000000000000}"
#define APOGUID_NOKEY L"!KEY"
#define APOGUID_NOVALUE L"!VALUE"

class DeviceAPOInfo : public AbstractAPOInfo
{
public:
	enum InstallMode
	{
		INSTALL_LFX_GFX = 0,
		INSTALL_SFX_MFX = 1,
		INSTALL_SFX_EFX = 2
	};

	struct InstallState
	{
		bool installPreMix;
		bool installPostMix;
		bool useOriginalAPOPreMix;
		bool useOriginalAPOPostMix;
		bool autoAdjust;
		InstallMode installMode;
		bool allowSilentBufferModification;

		InstallState()
		{
			installPreMix = false;
			installPostMix = false;
			useOriginalAPOPreMix = false;
			useOriginalAPOPostMix = false;
			autoAdjust = true;
			installMode = INSTALL_LFX_GFX;
			allowSilentBufferModification = false;
		}

		bool operator!=(const InstallState& other) const
		{
			return memcmp(this, &other, sizeof(InstallState)) != 0;
		}
	};

	static std::vector<std::shared_ptr<AbstractAPOInfo>> loadAllInfos(bool input);
	static std::wstring getDefaultDevice(bool input, int role = 1);
	static bool checkProtectedAudioDG(bool fix);
	static bool checkAPORegistration(bool fix);
	bool load(const std::wstring& deviceGuid, std::wstring defaultDeviceGuid = L"");
	bool canBeUpgraded() const override;
	bool hasChanges() const override;
	bool isExperimental() const override;
	std::wstring getOriginalAPOPreMix();
	std::wstring getOriginalAPOPostMix();
	void install() override;
	void uninstall() override;
	void reinstall() override;
	std::wstring getConnectionName() const override;
	std::wstring getDeviceName() const override;
	std::wstring getDeviceGuid() const override;
	std::wstring getDeviceString() const override;
	unsigned getChannelCount() const override;
	unsigned getSampleRate() const override;
	unsigned long getChannelMask() const override;
	bool isInput() const override;
	bool isInstalled() const override;
	bool isEnhancementsDisabled() const override;
	bool isDefaultDevice() const override;
	bool isDisabled() const override;
	bool isUnplugged() const override;
	const InstallState& getCurrentInstallState();
	InstallState& getSelectedInstallState();
	std::wstring getPreMixChildGuid();
	std::wstring getPostMixChildGuid();
	void testAPOInstallation();

private:
	void fail(const std::wstring& functionName, HRESULT hr);

	std::wstring deviceName;
	std::wstring connectionName;
	std::wstring deviceGuid;
	unsigned channelCount;
	unsigned sampleRate;
	unsigned long channelMask;
	bool defaultDevice;
	bool enhancementsDisabled;
	bool disabled;
	bool unplugged;

	// used for creating child APO
	std::wstring preMixChildGuid;
	std::wstring postMixChildGuid;

	// used for uninstallation
	std::wstring originalApoGuids[5];

	bool input;
	bool installed;
	std::wstring version;
	InstallState currentInstallState;
	// selection in GUI
	InstallState selectedInstallState;
};

class DeviceException
{
public:
	DeviceException(const std::wstring& message)
		: message(message)
	{
	}

	std::wstring getMessage()
	{
		return message;
	}

private:
	std::wstring message;
};
