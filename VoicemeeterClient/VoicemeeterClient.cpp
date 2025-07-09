/*
	This file is part of Equalizer APO, a system-wide equalizer.
	Copyright (C) 2022  Jonas Thedering

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
#include <shellapi.h>
#include "helpers/RegistryHelper.h"
#include "VoicemeeterClient.h"
#include "VoicemeeterAPOInfo.h"

#define voicemeeterKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define voicemeeterWowKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define uninstallStringValueName L"UninstallString"
#ifdef _WIN64
#define voicemeeterRemoteFileName L"VoicemeeterRemote64.dll"
#else
#define voicemeeterRemoteFileName L"VoicemeeterRemote.dll"
#endif
#define IDM_RESTART 200

using namespace std;

static long __stdcall callback(void* lpUser, long nCommand, void* lpData, long nnn);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd)
{
	vector<wstring> outputs;
	if (lpCmdLine[0] != 0)
	{
		int argc;
		wchar_t** argv = CommandLineToArgvW(lpCmdLine, &argc);
		for (int i = 0; i < argc; i++)
			outputs.push_back(argv[i]);
		LocalFree(argv);
	}

	try
	{
		VoicemeeterClient client(outputs);
		client.run();
		return 0;
	}
	catch (InitError e)
	{
		MessageBoxW(NULL, e.getMessage().c_str(), L"Equalizer APO Voicemeeter Client Initialization Error", MB_APPLMODAL | MB_OK | MB_ICONERROR);
		return -1;
	}
}

VoicemeeterClient::VoicemeeterClient(const vector<wstring>& outputs)
	: outputs(outputs)
{
	mainThreadId = GetCurrentThreadId();

	wstring voicemeeterDirectory;
	if (RegistryHelper::keyExists(voicemeeterKeyPath))
		voicemeeterDirectory = RegistryHelper::readValue(voicemeeterKeyPath, uninstallStringValueName);
	else if (RegistryHelper::keyExists(voicemeeterWowKeyPath))
		voicemeeterDirectory = RegistryHelper::readValue(voicemeeterWowKeyPath, uninstallStringValueName);

	size_t index = voicemeeterDirectory.find_last_of(L'\\');
	if (index != wstring::npos)
		voicemeeterDirectory = voicemeeterDirectory.substr(0, index);
	else
		throw InitError(L"Voicemeeter is not installed");

	HMODULE module = NULL;
	module = LoadLibraryW((voicemeeterDirectory + L"\\" voicemeeterRemoteFileName).c_str());
	if (module == NULL)
		throw InitError(L"Failed to load " voicemeeterRemoteFileName);

	memset(&vmr, 0, sizeof(T_VBVMR_INTERFACE));

#define LOAD_PROC(proc) vmr.proc = (T_ ## proc)GetProcAddress(module, # proc);if (vmr.proc == NULL) throw InitError(L"Did not find function \"" # proc L"\" in " voicemeeterRemoteFileName)
	LOAD_PROC(VBVMR_Login);
	LOAD_PROC(VBVMR_Logout);
	LOAD_PROC(VBVMR_GetVoicemeeterType);
	LOAD_PROC(VBVMR_IsParametersDirty);
	LOAD_PROC(VBVMR_AudioCallbackRegister);
	LOAD_PROC(VBVMR_AudioCallbackStart);
	LOAD_PROC(VBVMR_AudioCallbackStop);
	LOAD_PROC(VBVMR_AudioCallbackUnregister);

	initSoftware();
}

VoicemeeterClient::~VoicemeeterClient()
{
	if (wTimer != 0)
	{
		KillTimer(NULL, wTimer);
		wTimer = 0;
	}
	endSoftware();
}

void VoicemeeterClient::run()
{
	wTimer = SetTimer(NULL, 0, 500, NULL);
	PostThreadMessage(mainThreadId, WM_COMMAND, IDM_RESTART, 0);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		switch (msg.message)
		{
		case WM_COMMAND:
			handleCommand(msg.wParam, msg.lParam);
			break;
		case WM_TIMER:
			if (msg.wParam == wTimer)
			{
				// check if Voicemeeter type has changed
				if (vmr.VBVMR_IsParametersDirty() >= 0)
				{
					if (!connected)
						detectVoicemeeterType();
				}
				else
				{
					// Voicemeeter has been shut down
					connected = false;
				}
			}
			break;
		}
	}
}

void VoicemeeterClient::handle(long nCommand, void* lpData, long nnn)
{
	switch (nCommand)
	{
	case VBVMR_CBCOMMAND_STARTING:
	{
		VBVMR_LPT_AUDIOINFO audioInfo = (VBVMR_LPT_AUDIOINFO)lpData;
		sampleRate = (float)audioInfo->samplerate;
		maxFrameCount = audioInfo->nbSamplePerFrame;
		for (FilterEngine* engine : engines)
			if (engine != NULL)
				engine->initialize(sampleRate, 8, 8, 8, 0, maxFrameCount);
		VoicemeeterAPOInfo::saveVoicemeeterSampleRate((unsigned)audioInfo->samplerate);
	}
	break;
	case VBVMR_CBCOMMAND_ENDING:
		break;
	case VBVMR_CBCOMMAND_CHANGE:
		PostThreadMessage(mainThreadId, WM_COMMAND, IDM_RESTART, 0);
		break;
	case VBVMR_CBCOMMAND_BUFFER_OUT:
	{
		VBVMR_LPT_AUDIOBUFFER audioBuffer = (VBVMR_LPT_AUDIOBUFFER)lpData;
		int nbi = audioBuffer->audiobuffer_nbi;
		int nbo = audioBuffer->audiobuffer_nbo;
		unsigned n = min(nbi, nbo) / 8;

		for (unsigned i = 0; i < n; i++)
		{
			FilterEngine* engine = NULL;
			if (i < engines.size())
				engine = engines[i];

			bool idle = true;
			bool inputSilent = true;
			if (engine != NULL)
			{
				inputSilent = isBufferSilent(audioBuffer->audiobuffer_r + 8 * i, audioBuffer->audiobuffer_nbs);
				idle = inputSilent && idleSampleCounts[i] > 10 * engine->getSampleRate();
			}

			// avoid processing when idle (Voicemeeter does still call this when no audio is played)
			if (!idle)
			{
				engine->process(audioBuffer->audiobuffer_w + 8 * i, audioBuffer->audiobuffer_r + 8 * i, audioBuffer->audiobuffer_nbs);

				bool outputSilent = isBufferSilent(audioBuffer->audiobuffer_w + 8 * i, audioBuffer->audiobuffer_nbs);
				if (inputSilent && outputSilent)
					idleSampleCounts[i] += audioBuffer->audiobuffer_nbs;
				else
					idleSampleCounts[i] = 0;
			}
			else
			{
				for (int j = 0; j < 8; j++)
					memcpy(audioBuffer->audiobuffer_w[8 * i + j], audioBuffer->audiobuffer_r[8 * i + j], audioBuffer->audiobuffer_nbs * sizeof(float));
			}
		}
	}
	break;
	}
}

void VoicemeeterClient::initSoftware()
{
	long rep = vmr.VBVMR_Login();
	if (rep < 0)
		throw InitError(L"Failed To Login");
	if (vmr.VBVMR_IsParametersDirty() == 0)
		detectVoicemeeterType();
	else
		connected = false;
	unsigned tries = 30;
	bool loop = true;
	while (loop)
	{
		loop = false;
		char clientName[64] = "Equalizer APO";
		rep = vmr.VBVMR_AudioCallbackRegister(VBVMR_AUDIOCALLBACK_OUT, callback, this, clientName);
		if (rep == 1)
		{
			wchar_t message[512];
			wsprintf(message, L"Voicemeeter Output Insert already in use by:\n%S", clientName);
			throw InitError(message);
		}
		else if (rep != 0)
		{
			if (tries > 1)
			{
				// sometimes fails temporarily after restarting VoicemeeterClient
				Sleep(100);
				tries--;
				loop = true;
			}
			else
			{
				throw InitError(L"Failed to register audio callback");
			}
		}
	}
}

void VoicemeeterClient::detectVoicemeeterType()
{
	long vmType;
	long rep = vmr.VBVMR_GetVoicemeeterType(&vmType);
	if (rep == 0)
	{
		connected = true;

		unsigned outputCount;
		if (vmType == 3)
			outputCount = 5;
		else if (vmType == 2)
			outputCount = 3;
		else
			outputCount = 1;

		if (outputCount != engines.size())
		{
			idleSampleCounts.clear();
			for (FilterEngine* engine : engines)
				if (engine != NULL)
					delete engine;
			engines.clear();

			for (unsigned i = 0; i < outputCount; i++)
			{
				wstringstream sstream;
				sstream << "Output A" << (i + 1);
				wstring output = sstream.str();
				if (find(outputs.begin(), outputs.end(), output) != outputs.end())
				{
					FilterEngine* engine = new FilterEngine();
					engine->setDeviceInfo(false, true, L"Voicemeeter", output, L"", L"Voicemeeter " + output);
					if (sampleRate != 0.0f && maxFrameCount != 0)
						engine->initialize(sampleRate, 8, 8, 8, 0, maxFrameCount);
					engines.push_back(engine);
				}
				else
				{
					engines.push_back(NULL);
				}

				idleSampleCounts.push_back(0);
			}
		}
	}
}

void VoicemeeterClient::endSoftware()
{
	if (vmr.VBVMR_Logout != NULL)
		vmr.VBVMR_Logout();
	if (vmr.VBVMR_AudioCallbackUnregister != NULL)
		vmr.VBVMR_AudioCallbackUnregister();
}

void VoicemeeterClient::handleCommand(WPARAM wparam, LPARAM lparam)
{
	switch (LOWORD(wparam))
	{
	case IDM_RESTART:
		Sleep(50);
		if (vmr.VBVMR_AudioCallbackStart != NULL)
			vmr.VBVMR_AudioCallbackStart();
		break;
	}
}

bool VoicemeeterClient::isBufferSilent(float** sampleData, long sampleCount)
{
	bool silent = true;

	for (int j = 0; j < 8; j++)
	{
		float* buf = sampleData[j];
		for (int k = 0; k < sampleCount; k++)
		{
			if (buf[k] != 0.0f)
			{
				silent = false;
				break;
			}
		}
	}

	return silent;
}

static long __stdcall callback(void* lpUser, long nCommand, void* lpData, long nnn)
{
	VoicemeeterClient* client = (VoicemeeterClient*)lpUser;
	client->handle(nCommand, lpData, nnn);

	return 0;
}
