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
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "EqualizerAPO.h"
#include "ClassFactory.h"
#include "helpers/RegistryHelper.h"
#include "helpers/LogHelper.h"

using namespace std;

static HINSTANCE hModule;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, void* lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		::hModule = hModule;

	return TRUE;
}

STDAPI DllCanUnloadNow()
{
	if (EqualizerAPO::instCount == 0 && ClassFactory::lockCount == 0)
		return S_OK;
	else
		return S_FALSE;
}

STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void** ppv)
{
	if (clsid != EQUALIZERAPO_POST_MIX_GUID && clsid != EQUALIZERAPO_PRE_MIX_GUID)
		return CLASS_E_CLASSNOTAVAILABLE;

	ClassFactory* factory = new ClassFactory();
	if (factory == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = factory->QueryInterface(iid, ppv);
	factory->Release();

	return hr;
}

STDAPI DllRegisterServer()
{
	wchar_t filename[1024];
	GetModuleFileNameW(hModule, filename, sizeof(filename) / sizeof(wchar_t));

	HRESULT hr = RegisterAPO(EqualizerAPO::regPostMixProperties);
	if (FAILED(hr))
	{
		UnregisterAPO(EQUALIZERAPO_POST_MIX_GUID);
		return hr;
	}

	hr = RegisterAPO(EqualizerAPO::regPreMixProperties);
	if (FAILED(hr))
	{
		UnregisterAPO(EQUALIZERAPO_POST_MIX_GUID);
		UnregisterAPO(EQUALIZERAPO_PRE_MIX_GUID);
		return hr;
	}

	try
	{
		wstring apoClsidString = RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID);

		RegistryHelper::createKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString);
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString, L"", L"EqualizerAPO Post-Mix Class");
		RegistryHelper::createKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"", filename);
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"ThreadingModel", L"Both");

		apoClsidString = RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID);

		RegistryHelper::createKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString);
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString, L"", L"EqualizerAPO Pre-Mix Class");
		RegistryHelper::createKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"", filename);
		RegistryHelper::writeValue(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32", L"ThreadingModel", L"Both");
	}
	catch (RegistryException e)
	{
		UnregisterAPO(EQUALIZERAPO_POST_MIX_GUID);
		UnregisterAPO(EQUALIZERAPO_PRE_MIX_GUID);
		return E_FAIL;
	}

	return S_OK;
}

STDAPI DllUnregisterServer()
{
	try
	{
		wstring apoClsidString = RegistryHelper::getGuidString(EQUALIZERAPO_POST_MIX_GUID);

		RegistryHelper::deleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::deleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString);

		apoClsidString = RegistryHelper::getGuidString(EQUALIZERAPO_PRE_MIX_GUID);

		RegistryHelper::deleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString + L"\\InprocServer32");
		RegistryHelper::deleteKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CLSID\\" + apoClsidString);
	}
	catch (RegistryException e)
	{
		return E_FAIL;
	}

	HRESULT hr = UnregisterAPO(EQUALIZERAPO_POST_MIX_GUID);
	UnregisterAPO(EQUALIZERAPO_PRE_MIX_GUID);

	return hr;
}
