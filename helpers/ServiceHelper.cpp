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
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "PrecisionTimer.h"
#include "StringHelper.h"
#include "ServiceHelper.h"

using namespace std;

void ServiceHelper::restartService(const wstring& serviceName)
{
	SC_HANDLE scManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (scManager == NULL)
		throw ServiceException(L"OpenSCManager failed (" + StringHelper::getSystemErrorString(GetLastError()) + L")");
	SCOPE_EXIT{CloseServiceHandle(scManager); };

	vector<shared_ptr<Service>> services;
	shared_ptr<Service> mainService(new Service(scManager, serviceName, true));
	services.push_back(mainService);

	DWORD mainState = mainService->getState();
	if (mainState == SERVICE_RUNNING)
	{
		vector<wstring> dependentServices = mainService->getActiveDependentServices();
		for (wstring dependentServiceName : dependentServices)
		{
			shared_ptr<Service> dependentService(new Service(scManager, dependentServiceName.c_str(), false));
			services.insert(prev(services.end()), dependentService);
		}
	}

	PrecisionTimer timer;
	timer.start();
	for (shared_ptr<Service> service : services)
	{
		DWORD state = service->getState();
		if (state == SERVICE_RUNNING)
			state = service->stop();

		while (state != SERVICE_STOPPED)
		{
			if (timer.stop() > 30)
				throw ServiceException(L"Service stop timed out on service \"" + service->getServiceName() + L"\"");

			Sleep(100);

			state = service->getState();
		}
	}

	// all services should be stopped now, so start them again

	for (auto it = services.rbegin(); it != services.rend(); it++)
	{
		shared_ptr<Service> service = *it;
		service->start();

		DWORD state = service->getState();
		double retryDelay = 5;
		while (state != SERVICE_RUNNING)
		{
			double time = timer.stop();
			if (time > 30)
				throw ServiceException(L"Service start timed out on service \"" + service->getServiceName() + L"\"");
			if (time > retryDelay)
			{
				// sometimes, the service won't start on the first try
				service->start();
				retryDelay = time + 5;
			}

			Sleep(100);

			state = service->getState();
		}
	}
}

Service::Service(SC_HANDLE scManager, const std::wstring& serviceName, bool allowEnumerate)
	: serviceName(serviceName)
{
	DWORD desiredAccess = SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS;
	if (allowEnumerate)
		desiredAccess |= SERVICE_ENUMERATE_DEPENDENTS;
	serviceHandle = OpenServiceW(scManager, serviceName.c_str(), SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
	if (serviceHandle == NULL)
		fail(L"OpenService", GetLastError());
}

Service::~Service()
{
	CloseServiceHandle(serviceHandle);
}

const std::wstring& Service::getServiceName()
{
	return serviceName;
}

DWORD Service::getState()
{
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwBytesNeeded;
	if (!QueryServiceStatusEx(serviceHandle, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		fail(L"QueryServiceStatusEx", GetLastError());

	return ssp.dwCurrentState;
}

void Service::start()
{
	if (!StartServiceW(serviceHandle, 0, NULL))
		fail(L"StartService", GetLastError());
}

DWORD Service::stop()
{
	SERVICE_STATUS ss;
	if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &ss))
		fail(L"ControlService", GetLastError());

	return ss.dwCurrentState;
}

vector<wstring> Service::getActiveDependentServices()
{
	DWORD bytesNeeded, count;
	if (EnumDependentServicesW(serviceHandle, SERVICE_ACTIVE, NULL, 0, &bytesNeeded, &count))
		// if the call succeeds, there are no dependent services
		return vector<wstring>();

	DWORD error = GetLastError();
	if (error != ERROR_MORE_DATA)
		fail(L"EnumDependentServices", error);

	LPENUM_SERVICE_STATUSW dependencies = (LPENUM_SERVICE_STATUSW)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytesNeeded);
	if (!dependencies)
		throw ServiceException(L"HeapAlloc for EnumDependentServices failed");

	SCOPE_EXIT{HeapFree(GetProcessHeap(), 0, dependencies); };

	if (!EnumDependentServicesW(serviceHandle, SERVICE_ACTIVE, dependencies, bytesNeeded, &bytesNeeded, &count))
		fail(L"EnumDependentServices", GetLastError());

	vector<wstring> result;
	for (unsigned i = 0; i < count; i++)
		result.push_back(dependencies[i].lpServiceName);

	return result;
}

void Service::fail(const wstring& functionName, DWORD error)
{
	throw ServiceException(functionName + L" failed for service \"" + serviceName + L"\" (" + StringHelper::getSystemErrorString(error) + L")");
}
