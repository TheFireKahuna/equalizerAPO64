/*
    This file is part of Equalizer APO, a system-wide equalizer.
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
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include "helpers/RegistryHelper.h"
#include "VoicemeeterClient.h"
#include "VoicemeeterAPOInfo.h"

#define voicemeeterKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define voicemeeterWowKeyPath L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VB:Voicemeeter {17359A74-1236-5467}"
#define uninstallStringValueName L"UninstallString"

using namespace std;

long __stdcall callback(void* lpUser, long nCommand, void* lpData, long nnn);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR lpCmdLine,
	_In_ int nCmdShow)
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

	VoicemeeterClient* client = new VoicemeeterClient(outputs);

	MSG msg;
	msg.wParam = 0;

	bool loop = client->restart;
	while (loop)
	{
		client->start();
		client->restart = false;

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		loop = client->restart;

		client->stop();
	}
	delete client;

	return (int)msg.wParam;
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
	HMODULE voicemeeterModule = NULL;

	if (index != wstring::npos)
	{
		wstring setupFilename = voicemeeterDirectory.substr(index + 1);
		voicemeeterType = 1;
		if (setupFilename == L"VoicemeeterProSetup.exe")
			voicemeeterType = 2;// banana
		else if (setupFilename == L"Voicemeeter8Setup.exe")
			voicemeeterType = 3;// potato

		voicemeeterDirectory = voicemeeterDirectory.substr(0, index);

#ifdef _WIN64
		voicemeeterModule = LoadLibraryW((voicemeeterDirectory + L"\\VoicemeeterRemote64.dll").c_str());
#else
		voicemeeterModule = LoadLibraryW((voicemeeterDirectory + L"\\VoicemeeterRemote.dll").c_str());
#endif
	}

	memset(&vmr, 0, sizeof(T_VBVMR_INTERFACE));
#define LOAD_PROC(proc) vmr.proc = (T_ ## proc)GetProcAddress(voicemeeterModule, # proc)
	if (voicemeeterModule != NULL)
	{
		LOAD_PROC(VBVMR_Login);
		LOAD_PROC(VBVMR_Logout);
		LOAD_PROC(VBVMR_AudioCallbackRegister);
		LOAD_PROC(VBVMR_AudioCallbackStart);
		LOAD_PROC(VBVMR_AudioCallbackStop);
		LOAD_PROC(VBVMR_AudioCallbackUnregister);
		LOAD_PROC(VBVMR_GetVoicemeeterType);

		restart = true;
	}
}

VoicemeeterClient::~VoicemeeterClient()
{
	if (loggedIn)
	{
		vmr.VBVMR_Logout();
		loggedIn = false;
	}
}

void VoicemeeterClient::start()
{
	if (!loggedIn)
		loggedIn = (vmr.VBVMR_Login() == 0);

	if (loggedIn)
	{
		long type;
		if (vmr.VBVMR_GetVoicemeeterType(&type) == 0)
			voicemeeterType = type;
	}

	unsigned outputCount;
	if (voicemeeterType == 3)
		outputCount = 5;
	else if (voicemeeterType == 2)
		outputCount = 3;
	else
		outputCount = 1;

	for (unsigned i = 0; i < outputCount; i++)
	{
		wstringstream sstream;
		sstream << "Output A" << (i + 1);
		wstring output = sstream.str();
		if (find(outputs.begin(), outputs.end(), output) != outputs.end())
		{
			FilterEngine* engine = new FilterEngine();
			engine->setDeviceInfo(false, true, L"Voicemeeter", output, L"", L"Voicemeeter " + output);
			engines.push_back(engine);
		}
		else
		{
			engines.push_back(NULL);
		}

		idleSampleCounts.push_back(0);
	}

	if (vmr.VBVMR_AudioCallbackRegister(VBVMR_AUDIOCALLBACK_OUT, callback, this, (char*)"Equalizer APO") != 0)
	{
		PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);
		return;
	}
	vmr.VBVMR_AudioCallbackStart();
}

void VoicemeeterClient::stop()
{
	vmr.VBVMR_AudioCallbackStop();
	vmr.VBVMR_AudioCallbackUnregister();

	idleSampleCounts.clear();
	for (FilterEngine* engine : engines)
		if (engine != NULL)
			delete engine;
	engines.clear();
}

void VoicemeeterClient::handle(long nCommand, void* lpData, long nnn)
{
	switch (nCommand)
	{
	case VBVMR_CBCOMMAND_STARTING:
		{
			VBVMR_LPT_AUDIOINFO audioInfo = (VBVMR_LPT_AUDIOINFO)lpData;
			for (FilterEngine* engine : engines)
				if (engine != NULL)
					engine->initialize((float)audioInfo->samplerate, 8, 8, 8, 0, audioInfo->nbSamplePerFrame);
			VoicemeeterAPOInfo::saveVoicemeeterSampleRate((unsigned)audioInfo->samplerate);
		}
		break;
	case VBVMR_CBCOMMAND_ENDING:
		{
			restart = true;
			PostThreadMessage(mainThreadId, WM_QUIT, 0, 0);
		}
		break;
	case VBVMR_CBCOMMAND_CHANGE:
		{
			VBVMR_LPT_AUDIOINFO audioInfo = (VBVMR_LPT_AUDIOINFO)lpData;
			for (FilterEngine* engine : engines)
				if (engine != NULL)
					engine->initialize((float)audioInfo->samplerate, 8, 8, 8, 0, audioInfo->nbSamplePerFrame);
			VoicemeeterAPOInfo::saveVoicemeeterSampleRate((unsigned)audioInfo->samplerate);
		}
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

long __stdcall callback(void* lpUser, long nCommand, void* lpData, long nnn)
{
	VoicemeeterClient* client = (VoicemeeterClient*)lpUser;
	client->handle(nCommand, lpData, nnn);

	return 0;
}
