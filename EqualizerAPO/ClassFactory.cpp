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
#include "EqualizerAPO.h"
#include "../helpers/LogHelper.h"
#include "ClassFactory.h"

long ClassFactory::lockCount = 0;

ClassFactory::ClassFactory()
{
	refCount = 1;
}

HRESULT __stdcall ClassFactory::QueryInterface(const IID& iid, void** ppv)
{
	if (iid == __uuidof(IUnknown) || iid == __uuidof(IClassFactory))
		*ppv = static_cast<IClassFactory*>(this);
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

ULONG __stdcall ClassFactory::AddRef()
{
	return InterlockedIncrement(&refCount);
}

ULONG __stdcall ClassFactory::Release()
{
	if (InterlockedDecrement(&refCount) == 0)
	{
		delete this;
		return 0;
	}

	return refCount;
}

HRESULT __stdcall ClassFactory::CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv)
{
	if (pUnknownOuter != NULL && iid != __uuidof(IUnknown))
		return E_NOINTERFACE;

	EqualizerAPO* apo = new EqualizerAPO(pUnknownOuter);
	if (apo == NULL)
		return E_OUTOFMEMORY;

	HRESULT hr = apo->NonDelegatingQueryInterface(iid, ppv);

	apo->NonDelegatingRelease();
	return hr;
}

HRESULT __stdcall ClassFactory::LockServer(BOOL bLock)
{
	if (bLock)
		InterlockedIncrement(&lockCount);
	else
		InterlockedDecrement(&lockCount);

	return S_OK;
}
