/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

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
#include <ShlObj.h>
#include <fstream>
#include <algorithm>
#include <KsMedia.h>
#include <shellapi.h>
#include <TlHelp32.h>
#include <Winternl.h>
#include "helpers/RegistryHelper.h"
#include "VoicemeeterAPOInfo.h"

using namespace std;

#define voicemeeterKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define voicemeeterWowKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define uninstallStringValueName L"UninstallString"
static const wchar_t* startupFilename = L"Equalizer APO Voicemeeter Client.lnk";
static const wchar_t* clientFilename = L"VoicemeeterClient.exe";
static const wchar_t* voicemeeterClientKeyPath = USER_REGPATH L"\\Voicemeeter Client";
static const wchar_t* sampleRateValueName = L"sampleRate";

typedef NTSTATUS (NTAPI* pfnNtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

void VoicemeeterAPOInfo::prependInfos(vector<shared_ptr<AbstractAPOInfo> >& list)
{
	wstring voicemeeterDirectory;
	if (RegistryHelper::keyExists(voicemeeterKeyPath))
		voicemeeterDirectory = RegistryHelper::readValue(voicemeeterKeyPath, uninstallStringValueName);
	else if (RegistryHelper::keyExists(voicemeeterWowKeyPath))
		voicemeeterDirectory = RegistryHelper::readValue(voicemeeterWowKeyPath, uninstallStringValueName);

	if (voicemeeterDirectory.length() > 0)
	{
		size_t index = voicemeeterDirectory.find_last_of(L'\\');

		wstring setupFilename = voicemeeterDirectory.substr(index + 1);
		long voicemeeterType = 1;
		if (setupFilename == L"VoicemeeterProSetup.exe")
			voicemeeterType = 2;// banana
		else if (setupFilename == L"Voicemeeter8Setup.exe")
			voicemeeterType = 3;// potato

		unsigned outputCount;
		if (voicemeeterType == 3)
			outputCount = 5;
		else if (voicemeeterType == 2)
			outputCount = 3;
		else
			outputCount = 1;

		bool defaultDevice = false;
		list.erase(remove_if(list.begin(), list.end(), [&defaultDevice](shared_ptr<AbstractAPOInfo>& info) {
			if (info->getDeviceName().find(L"VB-Audio VoiceMeeter") != wstring::npos)
			{
				if (info->isDefaultDevice())
					defaultDevice = true;

				return true;
			}

			return false;
		}), list.end());

		bool anyInstalled = false;
		for (unsigned i = 0; i < outputCount; i++)
		{
			wstringstream sstream;
			sstream << "Output A" << (i + 1);
			shared_ptr<AbstractAPOInfo> info = *list.insert(list.begin() + i, make_shared<VoicemeeterAPOInfo>(sstream.str(), true));
			if (info->isInstalled())
				anyInstalled = true;
		}

		if (defaultDevice)
		{
			for (unsigned i = 0; i < outputCount; i++)
			{
				shared_ptr<VoicemeeterAPOInfo>& info = (shared_ptr<VoicemeeterAPOInfo>&)list[i];
				if (!anyInstalled || info->isInstalled())
				{
					info->defaultDevice = true;
					break;
				}
			}
		}
	}
	else
	{
		// Voicemeeter was uninstalled but Voicemeeter Client might still be installed
		wstring startupFilePath = getStartupPath();
		wstring argString = getLinkArgs(startupFilePath);
		vector<wstring> args = splitArgs(argString);

		int i = 0;
		for (wstring arg : args)
		{
			shared_ptr<AbstractAPOInfo> info = *list.insert(list.begin() + i, make_shared<VoicemeeterAPOInfo>(arg, false));
			i++;
		}
	}
}

VoicemeeterAPOInfo::VoicemeeterAPOInfo(const wstring& connectionName, bool voicemeeterInstalled)
	: connectionName(connectionName), voicemeeterInstalled(voicemeeterInstalled)
{
	wstring startupFilePath = getStartupPath();
	wstring path;
	wstring argString = getLinkArgs(startupFilePath, &path);
	vector<wstring> args = splitArgs(argString);
	installed = find(args.begin(), args.end(), connectionName) != args.end();

	sampleRate = 48000;
	try
	{
		if (RegistryHelper::keyExists(voicemeeterClientKeyPath) && RegistryHelper::valueExists(voicemeeterClientKeyPath, sampleRateValueName))
			sampleRate = (unsigned)RegistryHelper::readDWORDValue(voicemeeterClientKeyPath, sampleRateValueName);
	}
	catch (RegistryException e)
	{
		// ignore
	}

	wstring clientPath = getClientPath();
	changes = (path != clientPath);
}

wstring VoicemeeterAPOInfo::getConnectionName() const
{
	return connectionName;
}

wstring VoicemeeterAPOInfo::getDeviceName() const
{
	return L"Voicemeeter";
}

wstring VoicemeeterAPOInfo::getDeviceGuid() const
{
	return L"";
}

wstring VoicemeeterAPOInfo::getDeviceString() const
{
	return getConnectionName() + L" " + getDeviceName();
}

unsigned VoicemeeterAPOInfo::getChannelCount() const
{
	return 8;
}

unsigned VoicemeeterAPOInfo::getSampleRate() const
{
	return sampleRate;
}

unsigned long VoicemeeterAPOInfo::getChannelMask() const
{
	return KSAUDIO_SPEAKER_7POINT1_SURROUND;
}

bool VoicemeeterAPOInfo::isInput() const
{
	return false;
}

bool VoicemeeterAPOInfo::isInstalled() const
{
	return installed;
}

bool VoicemeeterAPOInfo::canBeUpgraded() const
{
	return false;
}

bool VoicemeeterAPOInfo::hasChanges() const
{
	return changes;
}

bool VoicemeeterAPOInfo::isExperimental() const
{
	return false;
}

bool VoicemeeterAPOInfo::isEnhancementsDisabled() const
{
	return false;
}

bool VoicemeeterAPOInfo::isDefaultDevice() const
{
	return defaultDevice;
}

bool VoicemeeterAPOInfo::isVoicemeeterInstalled() const
{
	return voicemeeterInstalled;
}

void VoicemeeterAPOInfo::install()
{
	wstring startupFilePath = getStartupPath();
	wstring argString = getLinkArgs(startupFilePath);
	vector<wstring> args = splitArgs(argString);
	vector<wstring>::iterator it = find(args.begin(), args.end(), connectionName);
	if (it == args.end())
		args.push_back(connectionName);
	sort(args.begin(), args.end());
	argString = joinArgs(args);

	wstring clientPath = getClientPath();

	createLink(startupFilePath, clientPath, argString);
}

void VoicemeeterAPOInfo::uninstall()
{
	wstring startupFilePath = getStartupPath();
	wstring argString = getLinkArgs(startupFilePath);
	vector<wstring> args = splitArgs(argString);
	vector<wstring>::iterator it = find(args.begin(), args.end(), connectionName);
	if (it != args.end())
		args.erase(it);
	if (args.size() > 0)
	{
		sort(args.begin(), args.end());
		argString = joinArgs(args);

		wchar_t filename[MAX_PATH];
		GetModuleFileNameW(NULL, filename, ARRAYSIZE(filename));
		PathRemoveFileSpecW(filename);
		wstring clientPath = filename;
		clientPath = clientPath + L"\\" + clientFilename;

		createLink(startupFilePath, clientPath, argString);
	}
	else
	{
		DeleteFileW(startupFilePath.c_str());
	}
}

void VoicemeeterAPOInfo::reinstall()
{
	uninstall();
	install();
}

wstring VoicemeeterAPOInfo::getStartupPath()
{
	PWSTR startupPath;
	SHGetKnownFolderPath(FOLDERID_Startup, KF_FLAG_DONT_UNEXPAND, NULL, &startupPath);
	wstring result(startupPath);

	CoTaskMemFree(startupPath);

	return result + L"\\" + startupFilename;
}

wstring VoicemeeterAPOInfo::getClientPath()
{
	wchar_t filename[MAX_PATH];
	GetModuleFileNameW(NULL, filename, ARRAYSIZE(filename));
	PathRemoveFileSpecW(filename);
	wstring clientPath = filename;
	clientPath = clientPath + L"\\" + clientFilename;

	return clientPath;
}

void VoicemeeterAPOInfo::createLink(const wstring& lnkPath, const wstring& path, const wstring& args)
{
	IShellLink* shellLink;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&shellLink);
	if (SUCCEEDED(hr))
	{
		shellLink->SetPath(path.c_str());
		shellLink->SetArguments(args.c_str());
		IPersistFile* persistFile;
		hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&persistFile);
		if (SUCCEEDED(hr))
		{
			hr = persistFile->Save(lnkPath.c_str(), TRUE);
			persistFile->Release();
		}

		shellLink->Release();
	}
}

wstring VoicemeeterAPOInfo::getLinkArgs(const wstring& lnkPath, wstring* path)
{
	wstring result;

	IShellLink* shellLink;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&shellLink);
	if (SUCCEEDED(hr))
	{
		IPersistFile* persistFile;
		hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&persistFile);
		if (SUCCEEDED(hr))
		{
			hr = persistFile->Load(lnkPath.c_str(), STGM_READ);
			if (SUCCEEDED(hr))
			{
				wchar_t buf[MAX_PATH];
				hr = shellLink->GetArguments(buf, ARRAYSIZE(buf));
				if (SUCCEEDED(hr))
					result = buf;

				if (path != NULL)
				{
					hr = shellLink->GetPath(buf, ARRAYSIZE(buf), NULL, 0);
					if (SUCCEEDED(hr))
						*path = buf;
				}
			}
			persistFile->Release();
		}

		shellLink->Release();
	}

	return result;
}

vector<wstring> VoicemeeterAPOInfo::splitArgs(const wstring& argString)
{
	vector<wstring> result;

	if (argString.length() > 0)
	{
		int argc;
		wchar_t** argv = CommandLineToArgvW(argString.c_str(), &argc);
		for (int i = 0; i < argc; i++)
			result.push_back(argv[i]);
		LocalFree(argv);
	}

	return result;
}

wstring VoicemeeterAPOInfo::joinArgs(const vector<wstring>& args)
{
	wstring result;
	for (const wstring& arg : args)
	{
		if (result.length() > 0)
			result += L" ";

		if (arg.find(' ') != wstring::npos)
			result += L"\"" + arg + L"\"";
		else
			result += arg;
	}

	return result;
}

void VoicemeeterAPOInfo::ensureVoicemeeterClientRunning()
{
	wstring startupFilePath = getStartupPath();
	wstring argString = getLinkArgs(startupFilePath);
	vector<wstring> args = splitArgs(argString);
	wstring clientPath = getClientPath();

	HANDLE tokenHandle;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
		throw RegistryException(L"Error in OpenProcessToken while taking ownership");

	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

	HMODULE module = LoadLibraryW(L"ntdll.dll");
	if (module == NULL)
		throw exception("Could not load ntdll.dll");
	SCOPE_EXIT {FreeLibrary(module);};

	pfnNtQueryInformationProcess NtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(module, "NtQueryInformationProcess");
	if (NtQueryInformationProcess == NULL)
		throw exception("Function NtQueryInformationProcess not found in ntdll.dll");

	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshotHandle == INVALID_HANDLE_VALUE)
		throw exception("Could not take a snapshot of all processes");
	SCOPE_EXIT {CloseHandle(snapshotHandle);};

	bool matchingProcessExists = false;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	bool loop = Process32FirstW(snapshotHandle, &entry) != 0;
	while (loop)
	{
		if (wcscmp(entry.szExeFile, clientFilename) == 0)
		{
			HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
			if (processHandle == INVALID_HANDLE_VALUE)
				throw exception("Could not open process");
			SCOPE_EXIT {CloseHandle(processHandle);};

			PROCESS_BASIC_INFORMATION basicInformation;
			NTSTATUS status = NtQueryInformationProcess(processHandle, ProcessBasicInformation, &basicInformation, sizeof(basicInformation), NULL);
			if (status < 0)
				throw exception("Could not query process information");

			_PEB peb;
			if (!ReadProcessMemory(processHandle, basicInformation.PebBaseAddress, &peb, sizeof(peb), NULL))
				throw exception("Could not read peb from process memory");

			RTL_USER_PROCESS_PARAMETERS processParams;
			if (!ReadProcessMemory(processHandle, peb.ProcessParameters, &processParams, sizeof(processParams), NULL))
				throw exception("Could not read process parameters from process memory");

			wchar_t* cmdLineBuf = new wchar_t[processParams.CommandLine.Length / sizeof(wchar_t)];
			SCOPE_EXIT {delete cmdLineBuf;};
			if (!ReadProcessMemory(processHandle, processParams.CommandLine.Buffer, cmdLineBuf, processParams.CommandLine.Length, NULL))
				throw exception("Could not read command line from process memory");
			wstring cmdLine(cmdLineBuf, processParams.CommandLine.Length / sizeof(wchar_t));

			vector<wstring> processArgs = splitArgs(cmdLine);
			wstring path = processArgs.front();
			processArgs.erase(processArgs.begin());

			if (path != clientPath || processArgs != args)
				closeProcess(entry.th32ProcessID);
			else
				matchingProcessExists = true;
		}

		loop = Process32NextW(snapshotHandle, &entry) != 0;
	}

	if (!matchingProcessExists && !args.empty())
	{
		ShellExecuteW(NULL, NULL, clientPath.c_str(), argString.c_str(), NULL, SW_SHOWDEFAULT);
	}
}

void VoicemeeterAPOInfo::saveVoicemeeterSampleRate(unsigned sampleRate)
{
	try
	{
		RegistryHelper::createKey(voicemeeterClientKeyPath);
		RegistryHelper::writeDWORDValue(voicemeeterClientKeyPath, sampleRateValueName, sampleRate);
	}
	catch (RegistryException e)
	{
		// ignore
	}
}

void VoicemeeterAPOInfo::closeProcess(unsigned long processId)
{
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshotHandle == INVALID_HANDLE_VALUE)
		throw exception("Could not take a snapshot of all processes");
	SCOPE_EXIT {CloseHandle(snapshotHandle);};

	THREADENTRY32 entry;
	entry.dwSize = sizeof(THREADENTRY32);
	bool loop = Thread32First(snapshotHandle, &entry) != 0;
	while (loop)
	{
		if (entry.th32OwnerProcessID == processId)
			// will just fail for threads not having a message queue
			PostThreadMessageW(entry.th32ThreadID, WM_QUIT, 0, 0);

		loop = Thread32Next(snapshotHandle, &entry) != 0;
	}
}
