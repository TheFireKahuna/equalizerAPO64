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
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <shellapi.h>
#include <comdef.h>

#include "DeviceAPOInfo.h"
#include "VoicemeeterAPOInfo.h"

#include "helpers/StringHelper.h"
#include "helpers/RegistryHelper.h"

using namespace std;

#define protectedDGKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Audio"
#define protectedDGValueName L"DisableProtectedAudioDG"
#define apoRegistrationKeyPath L"HKEY_CLASSES_ROOT\\AudioEngine\\AudioProcessingObjects"
#define clsidKeyPath L"HKEY_CLASSES_ROOT\\CLSID"
#define commonKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio"
#define renderKeyPath commonKeyPath L"\\Render"
#define captureKeyPath commonKeyPath L"\\Capture"
#define childApoPath APP_REGPATH L"\\Child APOs"
static const wchar_t* preMixChildGuidValueName = L"PreMixChild";
static const wchar_t* postMixChildGuidValueName = L"PostMixChild";
static const wchar_t* allowSilentBufferValueName = L"AllowSilentBufferModification";
static const wchar_t* disableAutoAdjustValueName = L"DisableAutomaticAdjustment";
static const wchar_t* versionValueName = L"Version";
static const wchar_t* connectionValueName = L"{a45c254e-df1c-4efd-8020-67d146a850e0},2";
static const wchar_t* deviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},6";
static const wchar_t* combinedDeviceValueName = L"{b3f8fa53-0004-438e-9003-51a46e139bfc},41";
static const wchar_t* formatValueName = L"{f19f064d-082c-4e27-bc73-6882a1bb8e4c},0";
static const wchar_t* channelMaskValueName = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e},3";
static const wchar_t* lfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},1";
static const wchar_t* gfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},2";
static const wchar_t* sfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},5";
static const wchar_t* mfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},6";
static const wchar_t* efxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},7";
static const wchar_t* multiSfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},13";
static const wchar_t* multiMfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},14";
static const wchar_t* multiEfxGuidValueName = L"{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},15";
static const unsigned allGuidValueNameCount = 5;
static const wchar_t* allGuidValueNames[] = {lfxGuidValueName, gfxGuidValueName, sfxGuidValueName, mfxGuidValueName, efxGuidValueName};
enum GuidValueIndices
{
	LFX_INDEX = 0,
	GFX_INDEX = 1,
	SFX_INDEX = 2,
	MFX_INDEX = 3,
	EFX_INDEX = 4
};
static const wchar_t* fxTitleValueName = L"{b725f130-47ef-101a-a5f1-02608c9eebac},10";
static const wchar_t* sfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},5";
static const wchar_t* mfxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},6";
static const wchar_t* efxProcessingModesValueName = L"{d3993a3f-99c2-4402-b5ec-a92a0367664b},7";
static const wchar_t* defaultProcessingModeValue = L"{C18E2F7E-933D-4965-B7D1-1EEF228D2AF3}";
static const wchar_t* disableEnhancementsValueName = L"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e},5";
static const wchar_t* installVersion = L"2";
static PROPERTYKEY guidPropertyKey = {{0x1da5d803, 0xd492, 0x4edd, 0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}, 4};

vector<shared_ptr<AbstractAPOInfo>> DeviceAPOInfo::loadAllInfos(bool input)
{
	vector<shared_ptr<AbstractAPOInfo>> result;

	vector<wstring> deviceGuidStrings = RegistryHelper::enumSubKeys(input ? captureKeyPath : renderKeyPath);
	wstring defaultDeviceGuid = getDefaultDevice(input);
	for (vector<wstring>::iterator it = deviceGuidStrings.begin(); it != deviceGuidStrings.end(); it++)
	{
		wstring deviceGuidString = *it;

		shared_ptr<DeviceAPOInfo> info = make_shared<DeviceAPOInfo>();
		if (info->load(deviceGuidString, defaultDeviceGuid))
		{
			info->selectedInstallState = info->currentInstallState;
			result.push_back(move(info));
		}
	}

	if (!input)
		VoicemeeterAPOInfo::prependInfos(result);

	return result;
}

wstring DeviceAPOInfo::getDefaultDevice(bool input, int role)
{
	wstring result;

	IMMDeviceEnumerator* enumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	if (SUCCEEDED(hr))
	{
		IMMDevice* endPoint = NULL;
		hr = enumerator->GetDefaultAudioEndpoint(input ? eCapture : eRender, (ERole)role, &endPoint);
		if (SUCCEEDED(hr))
		{
			IPropertyStore* propertyStore = NULL;
			hr = endPoint->OpenPropertyStore(STGM_READ, &propertyStore);
			if (SUCCEEDED(hr))
			{
				PROPVARIANT variant;
				PropVariantInit(&variant);
				hr = propertyStore->GetValue(guidPropertyKey, &variant);
				if (SUCCEEDED(hr))
				{
					result = variant.pwszVal;
					PropVariantClear(&variant);
				}
				propertyStore->Release();
			}
			endPoint->Release();
		}
		enumerator->Release();
	}

	return result;
}

bool DeviceAPOInfo::checkProtectedAudioDG(bool fix)
{
	bool result = true;

	if (!RegistryHelper::valueExists(protectedDGKeyPath, protectedDGValueName) || RegistryHelper::readDWORDValue(protectedDGKeyPath, protectedDGValueName) != 1)
	{
		result = false;

		if (fix)
			RegistryHelper::writeDWORDValue(protectedDGKeyPath, protectedDGValueName, 1);
	}

	return result;
}

bool DeviceAPOInfo::checkAPORegistration(bool fix)
{
	bool result = true;

	if (!RegistryHelper::keyExists(apoRegistrationKeyPath L"\\" + RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID))
		|| !RegistryHelper::keyExists(apoRegistrationKeyPath L"\\" + RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID))
		|| !RegistryHelper::keyExists(clsidKeyPath L"\\" + RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID))
		|| !RegistryHelper::keyExists(clsidKeyPath L"\\" + RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID)))
	{
		result = false;

		if (fix)
		{
			wchar_t path[MAX_PATH];
			if (GetModuleFileNameW(NULL, path, MAX_PATH) != 0)
			{
				PathRemoveFileSpecW(path);
				wstring params = wstring(L"/s \"") + path + L"\\EqualizerAPO.dll\"";
				ShellExecuteW(NULL, L"open", L"regsvr32.exe", params.c_str(), NULL, SW_SHOWNORMAL);
			}
		}
	}

	return result;
}

bool DeviceAPOInfo::load(const wstring& deviceGuid, wstring defaultDeviceGuid)
{
	wstring keyPath;
	if (RegistryHelper::keyExists(renderKeyPath L"\\" + deviceGuid))
	{
		keyPath = renderKeyPath L"\\" + deviceGuid;
		input = false;
	}
	else
	{
		keyPath = captureKeyPath L"\\" + deviceGuid;
		input = true;
	}

	unsigned long deviceState = RegistryHelper::readDWORDValue(keyPath, L"DeviceState");
	if (deviceState & DEVICE_STATE_NOTPRESENT)
		return false;
	// somehow, disabled devices do not actually use DEVICE_STATE_DISABLED in registry but 0x10000000
	disabled = deviceState & DEVICE_STATE_DISABLED || deviceState & 0x10000000;
	unplugged = deviceState & DEVICE_STATE_UNPLUGGED;

	this->deviceGuid = deviceGuid;

	connectionName = RegistryHelper::readValue(keyPath + L"\\Properties", connectionValueName);
	deviceName = RegistryHelper::readValue(keyPath + L"\\Properties", deviceValueName);

	channelCount = 0;
	sampleRate = 0;
	channelMask = 0;
	if (RegistryHelper::valueExists(keyPath + L"\\Properties", formatValueName))
	{
		std::vector<unsigned char> format = RegistryHelper::readBinaryValue(keyPath + L"\\Properties", formatValueName);
		if (format.size() >= sizeof(WAVEFORMATEX) + 8)
		{
			WAVEFORMATEX* waveFormat = (WAVEFORMATEX*)&format[8];
			channelCount = waveFormat->nChannels;
			sampleRate = waveFormat->nSamplesPerSec;
			if (waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			{
				WAVEFORMATEXTENSIBLE* waveFormatExtensible = (WAVEFORMATEXTENSIBLE*)waveFormat;
				channelMask = waveFormatExtensible->dwChannelMask;
			}
		}
	}
	if (channelMask == 0 && RegistryHelper::valueExists(keyPath + L"\\Properties", channelMaskValueName))
		channelMask = RegistryHelper::readDWORDValue(keyPath + L"\\Properties", channelMaskValueName);

	if (defaultDeviceGuid == L"")
		defaultDeviceGuid = getDefaultDevice(input);

	GUID guid1, guid2;
	if (SUCCEEDED(CLSIDFromString(deviceGuid.c_str(), &guid1)) && SUCCEEDED(CLSIDFromString(defaultDeviceGuid.c_str(), &guid2)))
		defaultDevice = (guid1 == guid2) != 0;
	else
		defaultDevice = false;

	enhancementsDisabled = false;
	if (RegistryHelper::keyExists(keyPath + L"\\FxProperties") && RegistryHelper::valueExists(keyPath + L"\\FxProperties", disableEnhancementsValueName))
		enhancementsDisabled = RegistryHelper::readDWORDValue(keyPath + L"\\FxProperties", disableEnhancementsValueName) != 0;

	installed = false;
	currentInstallState.installMode = INSTALL_LFX_GFX;
	version = L"0";
	preMixChildGuid = L"";
	postMixChildGuid = L"";
	currentInstallState.installPreMix = true;
	currentInstallState.installPostMix = !input;
	currentInstallState.useOriginalAPOPreMix = true;
	currentInstallState.useOriginalAPOPostMix = !input;
	currentInstallState.allowSilentBufferModification = false;
	currentInstallState.autoAdjust = true;

	if (!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
	{
		for (int i = 0; i < allGuidValueNameCount; i++)
			originalApoGuids[i] = APOGUID_NOKEY;
	}
	else
	{
		for (int i = 0; i < allGuidValueNameCount; i++)
		{
			if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
			{
				wstring originalApoGuid = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
				if (originalApoGuid == RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID) || originalApoGuid == RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID))
					originalApoGuid = APOGUID_NOVALUE;
				originalApoGuids[i] = originalApoGuid;
			}
			else
			{
				originalApoGuids[i] = APOGUID_NOVALUE;
			}
		}

		bool found = false;
		bool foundAt[allGuidValueNameCount];
		for (int i = 0; i < allGuidValueNameCount; i++)
		{
			foundAt[i] = false;

			if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
			{
				wstring apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);

				GUID apoGuid;
				if (SUCCEEDED(CLSIDFromString(apoGuidString.c_str(), &apoGuid)))
				{
					if (apoGuid == EQUALIZERAPO_PRE_MIX_GUID || apoGuid == EQUALIZERAPO_POST_MIX_GUID)
					{
						foundAt[i] = true;
						found = true;
					}
				}
			}
		}

		if (found)
		{
			installed = true;

			if (RegistryHelper::keyExists(childApoPath L"\\" + deviceGuid))
			{
				if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, versionValueName))
				{
					version = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, versionValueName);
					if (version != installVersion)
						throw RegistryException(L"Unsupported version of APO installation detected! Please uninstall newer Equalizer APO before using this version of Device Selector.");
				}
				else
				{
					version = L"1";
				}

				for (int i = 0; i < allGuidValueNameCount; i++)
				{
					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, allGuidValueNames[i]))
						originalApoGuids[i] = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i]);
				}

				if (version == installVersion)
				{
					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName))
						preMixChildGuid = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName);
					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName))
						postMixChildGuid = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName);

					currentInstallState.installPreMix = foundAt[LFX_INDEX] || foundAt[SFX_INDEX];
					currentInstallState.installPostMix = foundAt[GFX_INDEX] || foundAt[MFX_INDEX] || foundAt[EFX_INDEX];

					if (preMixChildGuid == L"")
						currentInstallState.useOriginalAPOPreMix = false;
					if (postMixChildGuid == L"")
						currentInstallState.useOriginalAPOPostMix = false;

					if (foundAt[LFX_INDEX] || foundAt[GFX_INDEX])
						currentInstallState.installMode = INSTALL_LFX_GFX;
					else if (foundAt[EFX_INDEX])
						currentInstallState.installMode = INSTALL_SFX_EFX;
					else if (foundAt[SFX_INDEX] || foundAt[MFX_INDEX])
						currentInstallState.installMode = INSTALL_SFX_MFX;

					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, allowSilentBufferValueName))
						currentInstallState.allowSilentBufferModification = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, allowSilentBufferValueName) != L"false";
					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, disableAutoAdjustValueName))
						currentInstallState.autoAdjust = RegistryHelper::readValue(childApoPath L"\\" + deviceGuid, disableAutoAdjustValueName) == L"false";
				}
				else
				{
					if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, lfxGuidValueName))
					{
						preMixChildGuid = originalApoGuids[0];
						postMixChildGuid = originalApoGuids[1];
					}
					else
					{
						preMixChildGuid = originalApoGuids[2];
						postMixChildGuid = originalApoGuids[3];
					}
				}
			}
			else if (RegistryHelper::keyExists(childApoPath) && RegistryHelper::valueExists(childApoPath, deviceGuid))
			{
				for (int i = 0; i < allGuidValueNameCount; i++)
				{
					if (foundAt[i])
					{
						originalApoGuids[i] = RegistryHelper::readValue(childApoPath, deviceGuid);
						if (i == LFX_INDEX || i == SFX_INDEX)
							preMixChildGuid = originalApoGuids[i];
						else
							postMixChildGuid = originalApoGuids[i];
						break;
					}
				}
			}
		}
		else
		{
			if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
			{
				// only use LFX/GFX if the audio driver supplied only those APOs
				if (RegistryHelper::keyExists(keyPath + L"\\FxProperties")
					&& (RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName) || RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxGuidValueName)
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", mfxGuidValueName)
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", efxGuidValueName)
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", multiSfxGuidValueName)
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", multiMfxGuidValueName)
					&& !RegistryHelper::valueExists(keyPath + L"\\FxProperties", multiEfxGuidValueName))
					currentInstallState.installMode = INSTALL_LFX_GFX;
				// bluetooth devices may be combined in Windows 11, EFX will not work then
				else if (RegistryHelper::valueExists(keyPath + L"\\Properties", combinedDeviceValueName))
					currentInstallState.installMode = INSTALL_SFX_MFX;
				else
					currentInstallState.installMode = INSTALL_SFX_EFX;
			}
		}
	}

	return true;
}

bool DeviceAPOInfo::canBeUpgraded() const
{
	return installed && version != installVersion;
}

bool DeviceAPOInfo::hasChanges() const
{
	return installed && selectedInstallState != currentInstallState;
}

bool DeviceAPOInfo::isExperimental() const
{
	return !installed && originalApoGuids[0] == APOGUID_NOKEY;
}

wstring DeviceAPOInfo::getOriginalAPOPreMix()
{
	wstring guid;
	switch (selectedInstallState.installMode)
	{
	case INSTALL_LFX_GFX:
		guid = originalApoGuids[LFX_INDEX];
		if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		{
			if (originalApoGuids[LFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[GFX_INDEX] == APOGUID_NOVALUE)
				guid = originalApoGuids[SFX_INDEX];
		}
		break;
	case INSTALL_SFX_MFX:
		guid = originalApoGuids[SFX_INDEX];
		if (originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[MFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[LFX_INDEX];
		break;
	case INSTALL_SFX_EFX:
		guid = originalApoGuids[SFX_INDEX];
		if (originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[EFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[LFX_INDEX];
		break;
	}

	if (guid == APOGUID_NOKEY || guid == APOGUID_NOVALUE)
		guid = L"";

	return guid;
}

wstring DeviceAPOInfo::getOriginalAPOPostMix()
{
	wstring guid;
	switch (selectedInstallState.installMode)
	{
	case INSTALL_LFX_GFX:
		guid = originalApoGuids[GFX_INDEX];
		if (RegistryHelper::isWindowsVersionAtLeast(6, 3)) // Windows 8.1
		{
			if (originalApoGuids[LFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[GFX_INDEX] == APOGUID_NOVALUE)
				guid = originalApoGuids[MFX_INDEX];
		}
		break;
	case INSTALL_SFX_MFX:
		guid = originalApoGuids[MFX_INDEX];
		if (originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[MFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[GFX_INDEX];
		break;
	case INSTALL_SFX_EFX:
		guid = originalApoGuids[EFX_INDEX];
		if (originalApoGuids[SFX_INDEX] == APOGUID_NOVALUE && originalApoGuids[EFX_INDEX] == APOGUID_NOVALUE)
			guid = originalApoGuids[GFX_INDEX];
		break;
	}

	if (guid == APOGUID_NOKEY || guid == APOGUID_NOVALUE)
		guid = L"";

	return guid;
}

void DeviceAPOInfo::install()
{
	if (!selectedInstallState.installPreMix && !selectedInstallState.installPostMix)
		return;

	RegistryHelper::createKey(childApoPath);
	RegistryHelper::createKey(childApoPath L"\\" + deviceGuid);

	wstring keyPath;
	if (!input)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if (!RegistryHelper::keyExists(keyPath + L"\\FxProperties"))
	{
		try
		{
			RegistryHelper::createKey(keyPath + L"\\FxProperties");
		}
		catch (RegistryException e)
		{
			// Permissions were not sufficient, so change them
			RegistryHelper::takeOwnership(keyPath);
			RegistryHelper::makeWritable(keyPath);

			RegistryHelper::createKey(keyPath + L"\\FxProperties");
		}

		RegistryHelper::writeValue(keyPath + L"\\FxProperties", fxTitleValueName, L"Equalizer APO");

		for (int i = 0; i < allGuidValueNameCount; i++)
		{
			RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i], APOGUID_NOKEY);
		}
	}
	else
	{
		vector<wstring> valuenames;

		for (int i = 0; i < allGuidValueNameCount; i++)
		{
			wstring apoGuidString = APOGUID_NOVALUE;
			if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
			{
				apoGuidString = RegistryHelper::readValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
				valuenames.push_back(allGuidValueNames[i]);
			}

			RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, allGuidValueNames[i], apoGuidString);
		}

		if (!valuenames.empty())
			RegistryHelper::saveToFile(keyPath + L"\\FxProperties", valuenames,
				L"backup_" + StringHelper::replaceIllegalCharacters(deviceName) + L"_" + StringHelper::replaceIllegalCharacters(connectionName) + L".reg");
	}

	wstring preMixValue;
	wstring postMixValue;
	if (selectedInstallState.useOriginalAPOPreMix)
		preMixValue = getOriginalAPOPreMix();
	if (selectedInstallState.useOriginalAPOPostMix)
		postMixValue = getOriginalAPOPostMix();
	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, preMixChildGuidValueName, preMixValue);
	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, postMixChildGuidValueName, postMixValue);

	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, allowSilentBufferValueName, selectedInstallState.allowSilentBufferModification ? L"true" : L"false");
	if (selectedInstallState.autoAdjust)
	{
		if (RegistryHelper::valueExists(childApoPath L"\\" + deviceGuid, disableAutoAdjustValueName))
			RegistryHelper::deleteValue(childApoPath L"\\" + deviceGuid, disableAutoAdjustValueName);
	}
	else
	{
		RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, disableAutoAdjustValueName, L"true");
	}
	RegistryHelper::writeValue(childApoPath L"\\" + deviceGuid, versionValueName, installVersion);

	if (selectedInstallState.installMode == INSTALL_LFX_GFX)
	{
		if (selectedInstallState.installPreMix)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", lfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
		if (selectedInstallState.installPostMix && !input)
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", gfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", sfxGuidValueName);
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", mfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", mfxGuidValueName);
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", efxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", efxGuidValueName);
	}
	else if (selectedInstallState.installMode == INSTALL_SFX_MFX)
	{
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", lfxGuidValueName);
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		if (selectedInstallState.installPreMix)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", sfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
			if (!RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", sfxProcessingModesValueName, defaultProcessingModeValue);
		}
		if (selectedInstallState.installPostMix && !input)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", mfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
			if (!RegistryHelper::valueExists(keyPath + L"\\FxProperties", mfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", mfxProcessingModesValueName, defaultProcessingModeValue);
		}
		// don't change efx
	}
	else if (selectedInstallState.installMode == INSTALL_SFX_EFX)
	{
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", lfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", lfxGuidValueName);
		if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", gfxGuidValueName))
			RegistryHelper::deleteValue(keyPath + L"\\FxProperties", gfxGuidValueName);
		if (selectedInstallState.installPreMix)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", sfxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID));
			if (!RegistryHelper::valueExists(keyPath + L"\\FxProperties", sfxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", sfxProcessingModesValueName, defaultProcessingModeValue);
		}
		// don't change mfx
		if (selectedInstallState.installPostMix && !input)
		{
			RegistryHelper::writeValue(keyPath + L"\\FxProperties", efxGuidValueName, RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID));
			if (!RegistryHelper::valueExists(keyPath + L"\\FxProperties", efxProcessingModesValueName))
				RegistryHelper::writeMultiValue(keyPath + L"\\FxProperties", efxProcessingModesValueName, defaultProcessingModeValue);
		}
	}

	// force-enable enhancements
	if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", disableEnhancementsValueName))
		RegistryHelper::deleteValue(keyPath + L"\\FxProperties", disableEnhancementsValueName);
}

void DeviceAPOInfo::uninstall()
{
	wstring keyPath;
	if (!input)
		keyPath = renderKeyPath L"\\" + deviceGuid;
	else
		keyPath = captureKeyPath L"\\" + deviceGuid;

	if (originalApoGuids[0] == APOGUID_NOKEY)
	{
		RegistryHelper::deleteKey(keyPath + L"\\FxProperties");
	}
	else
	{
		for (int i = 0; i < allGuidValueNameCount; i++)
		{
			if (originalApoGuids[i] == APOGUID_NOVALUE)
			{
				if (RegistryHelper::valueExists(keyPath + L"\\FxProperties", allGuidValueNames[i]))
					RegistryHelper::deleteValue(keyPath + L"\\FxProperties", allGuidValueNames[i]);
			}
			else if (originalApoGuids[i] != L"")
			{
				RegistryHelper::writeValue(keyPath + L"\\FxProperties", allGuidValueNames[i], originalApoGuids[i]);
			}
		}
	}

	if (RegistryHelper::keyExists(childApoPath) && RegistryHelper::valueExists(childApoPath, deviceGuid))
		RegistryHelper::deleteValue(childApoPath, deviceGuid);

	if (RegistryHelper::keyExists(childApoPath L"\\" + deviceGuid))
		RegistryHelper::deleteKey(childApoPath L"\\" + deviceGuid);

	if (RegistryHelper::keyExists(childApoPath) && RegistryHelper::keyEmpty(childApoPath))
		RegistryHelper::deleteKey(childApoPath);
}

void DeviceAPOInfo::reinstall()
{
	uninstall();
	load(deviceGuid);
	install();
}

wstring DeviceAPOInfo::getConnectionName() const
{
	return connectionName;
}

wstring DeviceAPOInfo::getDeviceName() const
{
	return deviceName;
}

wstring DeviceAPOInfo::getDeviceGuid() const
{
	return deviceGuid;
}

wstring DeviceAPOInfo::getDeviceString() const
{
	return getConnectionName() + L" " + getDeviceName() + L" " + getDeviceGuid();
}

unsigned DeviceAPOInfo::getChannelCount() const
{
	return channelCount;
}

unsigned DeviceAPOInfo::getSampleRate() const
{
	return sampleRate;
}

unsigned long DeviceAPOInfo::getChannelMask() const
{
	return channelMask;
}

bool DeviceAPOInfo::isInput() const
{
	return input;
}

bool DeviceAPOInfo::isInstalled() const
{
	return installed;
}

bool DeviceAPOInfo::isEnhancementsDisabled() const
{
	return enhancementsDisabled;
}

bool DeviceAPOInfo::isDefaultDevice() const
{
	return defaultDevice;
}

bool DeviceAPOInfo::isDisabled() const
{
	return disabled;
}

bool DeviceAPOInfo::isUnplugged() const
{
	return unplugged;
}

const DeviceAPOInfo::InstallState& DeviceAPOInfo::getCurrentInstallState()
{
	return currentInstallState;
}

DeviceAPOInfo::InstallState& DeviceAPOInfo::getSelectedInstallState()
{
	return selectedInstallState;
}

wstring DeviceAPOInfo::getPreMixChildGuid()
{
	return preMixChildGuid;
}

wstring DeviceAPOInfo::getPostMixChildGuid()
{
	return postMixChildGuid;
}

void DeviceAPOInfo::testAPOInstallation()
{
	IMMDeviceEnumerator* enumerator = NULL;
	IMMDevice* device = NULL;
	IAudioClient* audioClient = NULL;
	WAVEFORMATEX* format = NULL;

	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
	if (FAILED(hr))
		fail(L"CoCreateInstance for IMMDeviceEnumerator", hr);
	SCOPE_EXIT{enumerator->Release(); };

	hr = enumerator->GetDevice(((input ? L"{0.0.1.00000000}." : L"{0.0.0.00000000}.") + deviceGuid).c_str(), &device);
	if (FAILED(hr))
		fail(L"GetDevice", hr);
	SCOPE_EXIT{device->Release(); };

	DWORD state;
	hr = device->GetState(&state);
	if (FAILED(hr))
		fail(L"GetState", hr);
	if (state & DEVICE_STATE_DISABLED || state & DEVICE_STATE_UNPLUGGED)
		return;

	hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&audioClient);
	if (FAILED(hr))
		fail(L"Activate", hr);
	SCOPE_EXIT{audioClient->Release(); };

	hr = audioClient->GetMixFormat(&format);
	if (FAILED(hr))
		fail(L"GetMixFormat", hr);
	SCOPE_EXIT{CoTaskMemFree(format); };

	hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 1000000 /*100 ms*/, 0, format, NULL);
	if (FAILED(hr))
		fail(L"Initialize", hr);
}

void DeviceAPOInfo::fail(const wstring& functionName, HRESULT hr)
{
	_com_error err(hr);
	const wchar_t* msg = err.ErrorMessage();
	throw DeviceException(functionName + L" failed for device \"" + deviceName + L"\" (" + msg + L")");
}
