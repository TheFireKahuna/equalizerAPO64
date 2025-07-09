/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2024  Jonas Dahlinger

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
#define _WIN32_DCOM
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <comdef.h>
#include <taskschd.h>
#define SECURITY_WIN32
#include <Security.h>
#include "StringHelper.h"
#include "ScopeGuard.h"
#include "TaskSchedulerHelper.h"

void TaskSchedulerHelper::scheduleAtLogon(const std::wstring& taskName, const std::wstring& programPath, const std::wstring& programArgs, const std::wstring& workingDir)
{
	HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
		fail(L"CoInitializeSecurity", hr);

	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr))
		fail(L"CoCreateInstance", hr);
	SCOPE_EXIT{if (pService != NULL) pService->Release();};

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
		fail(L"ITaskService::Connect", hr);

	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
		fail(L"ITaskService::GetFolder", hr);
	SCOPE_EXIT{pRootFolder->Release(); };

	// possibly delete existing task
	pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);

	ITaskDefinition* pTask = NULL;
	hr = pService->NewTask(0, &pTask);

	pService->Release();
	pService = NULL;

	if (FAILED(hr))
		fail(L"ITaskService::NewTask", hr);
	SCOPE_EXIT{pTask->Release(); };

	wchar_t userName[255];
	ULONG userNameSize = sizeof(userName)/sizeof(wchar_t);
	if (!GetUserNameExW(NameSamCompatible, userName, &userNameSize))
		fail(L"GetUserNameExW", GetLastError());

	IRegistrationInfo* pRegInfo= NULL;
	hr = pTask->get_RegistrationInfo(&pRegInfo);
	if (FAILED(hr))
		fail(L"ITaskDefinition::get_RegistrationInfo", hr);

	hr = pRegInfo->put_Author(userName);
	pRegInfo->Release();
	if (FAILED(hr))
		fail(L"IRegistrationInfo::put_Author", hr);

	{
		ITaskSettings* pSettings = NULL;
		hr = pTask->get_Settings(&pSettings);
		if (FAILED(hr))
			fail(L"ITaskDefinition::get_Settings", hr);
		SCOPE_EXIT{pSettings->Release();};

		hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
		if (FAILED(hr))
			fail(L"ITaskSettings::put_StartWhenAvailable", hr);

		hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
		if (FAILED(hr))
			fail(L"ITaskSettings::put_DisallowStartIfOnBatteries", hr);

		hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
		if (FAILED(hr))
			fail(L"ITaskSettings::put_StopIfGoingOnBatteries", hr);

		hr = pSettings->put_RunOnlyIfNetworkAvailable(VARIANT_TRUE);
		if (FAILED(hr))
			fail(L"ITaskSettings::put_RunOnlyIfNetworkAvailable", hr);
	}

	ITriggerCollection* pTriggerCollection = NULL;
	hr = pTask->get_Triggers(&pTriggerCollection);
	if (FAILED(hr))
		fail(L"ITaskDefinition::get_Triggers", hr);

	ITrigger* pTrigger = NULL;
	hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
	pTriggerCollection->Release();
	if (FAILED(hr))
		fail(L"ITriggerCollection::Create", hr);

	{
		ILogonTrigger* pLogonTrigger = NULL;
		hr = pTrigger->QueryInterface(
			IID_ILogonTrigger, (void**)&pLogonTrigger);
		pTrigger->Release();
		if (FAILED(hr))
			fail(L"QueryInterface(ILogonTrigger)", hr);
		SCOPE_EXIT{pLogonTrigger->Release();};

		hr = pLogonTrigger->put_Id(_bstr_t(L"Trigger1"));
		if (FAILED(hr))
			fail(L"ITrigger::put_Id", hr);

		hr = pLogonTrigger->put_UserId(_bstr_t(userName));
		if (FAILED(hr))
			fail(L"ILogonTrigger::put_UserId", hr);

		IRepetitionPattern* repetitionPattern = NULL;
		hr = pLogonTrigger->get_Repetition(&repetitionPattern);
		if (FAILED(hr))
			fail(L"ILogonTrigger::get_Repetition", hr);

		hr = repetitionPattern->put_Interval(_bstr_t(L"P1D"));
		repetitionPattern->Release();
		if (FAILED(hr))
			fail(L"IRepetitionPattern::put_Interval", hr);
	}

	IActionCollection* pActionCollection = NULL;
	hr = pTask->get_Actions(&pActionCollection);
	if (FAILED(hr))
		fail(L"ITaskDefinition::get_Actions", hr);

	IAction* pAction = NULL;
	hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
	pActionCollection->Release();
	if (FAILED(hr))
		fail(L"IActionCollection::Create", hr);

	{
		IExecAction* pExecAction = NULL;
		hr = pAction->QueryInterface(
			IID_IExecAction, (void**)&pExecAction);
		pAction->Release();
		if (FAILED(hr))
			fail(L"QueryInterface(IExecAction)", hr);
		SCOPE_EXIT{pExecAction->Release(); };

		hr = pExecAction->put_Path(_bstr_t(programPath.c_str()));
		if (FAILED(hr))
			fail(L"IExecAction::put_Path", hr);

		hr = pExecAction->put_Arguments(_bstr_t(programArgs.c_str()));
		if (FAILED(hr))
			fail(L"IExecAction::put_Arguments", hr);

		hr = pExecAction->put_WorkingDirectory(_bstr_t(workingDir.c_str()));
		if (FAILED(hr))
			fail(L"IExecAction::put_WorkingDirectory", hr);
	}

	IRegisteredTask* pRegisteredTask = NULL;
	hr = pRootFolder->RegisterTaskDefinition(
		_bstr_t(taskName.c_str()),
		pTask,
		TASK_CREATE_OR_UPDATE,
		_variant_t(),
		_variant_t(),
		TASK_LOGON_INTERACTIVE_TOKEN,
		_variant_t(L""),
		&pRegisteredTask);
	if (FAILED(hr))
		fail(L"ITaskFolder::RegisterTaskDefinition", hr);
	pRegisteredTask->Release();
}

void TaskSchedulerHelper::unschedule(const std::wstring& taskName)
{
	HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
	if (FAILED(hr))
		fail(L"CoInitializeSecurity", hr);

	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
	if (FAILED(hr))
		fail(L"CoCreateInstance", hr);
	SCOPE_EXIT{pService->Release(); };

	hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
	if (FAILED(hr))
		fail(L"ITaskService::Connect", hr);

	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
	if (FAILED(hr))
		fail(L"ITaskService::GetFolder", hr);
	SCOPE_EXIT{pRootFolder->Release(); };

	// possibly delete existing task
	pRootFolder->DeleteTask(_bstr_t(taskName.c_str()), 0);
}

void TaskSchedulerHelper::fail(const std::wstring& functionName, unsigned long error)
{
	throw TaskSchedulerException(functionName + L" failed in task scheduling (" + StringHelper::getSystemErrorString(error) + L")");
}
