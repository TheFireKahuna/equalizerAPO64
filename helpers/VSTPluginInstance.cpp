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
#include <wincrypt.h>
#include "StringHelper.h"
#include "version.h"
#include "VSTPluginLibrary.h"
#include "VSTPluginInstance.h"

using namespace std;

#define equalizerApoVSTID CCONST('E', 'A', 'P', 'O');

static intptr_t callback(AEffect* effect, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt)
{
	VSTPluginInstance* instance = effect != NULL ? (VSTPluginInstance*)effect->user : NULL;
#ifdef _DEBUG
	printf("vst: %p opcode: %d index: %d value: %d ptr: %p opt: %f user: %p\n",
		effect, opcode, index, value, ptr, opt, effect != NULL ? effect->user : NULL);
	fflush(stdout);
#endif

	switch (opcode)
	{
	case audioMasterVersion:
		return 2400;

	case audioMasterCurrentId:
		return equalizerApoVSTID;

	case audioMasterGetProductString:
		strcpy_s((char*)ptr, 64, "Equalizer APO");
		return 1;

	case audioMasterGetVendorVersion:
		return (intptr_t)(MAJOR << 24 | MINOR << 16 | REVISION << 8 | 0);

	case audioMasterPinConnected:
		if (instance != NULL)
			return index < instance->getUsedChannelCount() ? 0 : 1;
		else
			return 0;

	case audioMasterNeedIdle:
		return effect->dispatcher(effect, effIdle, 0, 0, NULL, 0.0f);

	case audioMasterUpdateDisplay:
		return effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);

	case audioMasterGetTime:
		return 0;

	case audioMasterGetSampleRate:
		if (instance != NULL)
			return (intptr_t)instance->getSampleRate();
		return 0;

	case audioMasterGetCurrentProcessLevel:
		if (instance != NULL)
			return instance->getProcessLevel();
		return 0;

	case audioMasterGetLanguage:
		if (instance != NULL)
			return instance->getLanguage();
		return 0;

	case audioMasterWillReplaceOrAccumulate:
		return 1;

	case audioMasterSizeWindow:
		if (instance != NULL)
			instance->onSizeWindow((int)index, (int)value);
		return 1;

	case audioMasterCanDo:
	{
		char* s = (char*)ptr;
#ifdef _DEBUG
		printf("VST canDo: %s\n", s);
		fflush(stdout);
#endif
		if (strcmp(s, "startStopProcess") == 0 ||
			strcmp(s, "sizeWindow") == 0)
			return 1;
	}
	return 0;

	case audioMasterAutomate:
		if (instance != NULL)
			instance->onAutomate();
		return 0;

	case audioMasterBeginEdit:
	case audioMasterEndEdit:
	case audioMasterIdle:
	case audioMasterWantMidi:
		return 0;
	}

	return 0;
}

VSTPluginInstance::VSTPluginInstance(const std::shared_ptr<VSTPluginLibrary>& library, int processLevel)
	: library(library), processLevel(processLevel)
{
}

VSTPluginInstance::~VSTPluginInstance()
{
	if (effect != NULL)
	{
		effect->dispatcher(effect, effClose, 0, 0, NULL, 0.0f);
		effect = NULL;
	}
}

bool VSTPluginInstance::initialize()
{
	bool result = true;

	__try
	{
		effect = library->VSTPluginMain(callback);
		effect->user = this;
		if (effect->magic == kEffectMagic)
		{
			effect->dispatcher(effect, effOpen, 0, 0, NULL, 0.0f);

			usedChannelCount = max(numInputs(), numOutputs());
		}
		else
		{
			effect = NULL;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		result = false;
	}

	return result;
}

int VSTPluginInstance::numInputs() const
{
	if (effect == NULL)
		return 0;

	return effect->numInputs;
}

int VSTPluginInstance::numOutputs() const
{
	if (effect == NULL)
		return 0;

	return effect->numOutputs;
}

bool VSTPluginInstance::canReplacing() const
{
	if (effect == NULL)
		return true;

	return (effect->flags & effFlagsCanReplacing) != 0;
}

bool VSTPluginInstance::canDoubleReplacing() const
{
	if (effect == NULL)
		return true;

	return (effect->flags & effFlagsCanDoubleReplacing) != 0;
}

int VSTPluginInstance::uniqueID() const
{
	if (effect == NULL)
		return 0;

	return effect->uniqueID;
}

std::wstring VSTPluginInstance::getName() const
{
	if (effect == NULL)
		return L"";

	char buf[256];
	memset(buf, 0, sizeof(buf));
	effect->dispatcher(effect, effGetEffectName, 0, 0, buf, 0.0f);

	return StringHelper::toWString(buf, CP_UTF8);
}

int VSTPluginInstance::getUsedChannelCount() const
{
	return usedChannelCount;
}

void VSTPluginInstance::setUsedChannelCount(int count)
{
	usedChannelCount = count;
}

float VSTPluginInstance::getSampleRate() const
{
	return sampleRate;
}

int VSTPluginInstance::getProcessLevel() const
{
	return processLevel;
}

void VSTPluginInstance::setProcessLevel(int value)
{
	processLevel = value;
}

int VSTPluginInstance::getLanguage() const
{
	return language;
}

void VSTPluginInstance::setLanguage(int value)
{
	language = value;
}

void VSTPluginInstance::prepareForProcessing(float sampleRate, int blockSize)
{
	if (effect == NULL)
		return;

	this->sampleRate = sampleRate;
	effect->dispatcher(effect, effSetSampleRate, 0, 0, NULL, sampleRate);
	effect->dispatcher(effect, effSetBlockSize, 0, blockSize, NULL, 0.0f);
}

void VSTPluginInstance::writeToEffect(const std::wstring& chunkData, const std::unordered_map<std::wstring, float>& paramMap)
{
	if (effect == NULL)
		return;

	if (effect->flags & effFlagsProgramChunks)
	{
		if (chunkData != L"")
		{
			DWORD bufSize = 0;
			CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, NULL, &bufSize, NULL, NULL);
			BYTE* buf = new BYTE[bufSize];
			if (CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, buf, &bufSize, NULL, NULL) == TRUE)
				effect->dispatcher(effect, effSetChunk, 1, bufSize, buf, 0.0f);
			delete[] buf;
		}
	}
	else
	{
		for (int i = 0; i < effect->numParams; i++)
		{
			char buf[256];
			effect->dispatcher(effect, effGetParamName, i, 0, buf, 0.0f);
			wstring name = StringHelper::toWString(buf, CP_UTF8);
			auto it = paramMap.find(name);
			if (it != paramMap.end())
				effect->setParameter(effect, i, it->second);
		}
	}
}

void VSTPluginInstance::readFromEffect(std::wstring& chunkData, std::unordered_map<std::wstring, float>& paramMap) const
{
	if (effect == NULL)
		return;

	chunkData = L"";
	paramMap.clear();

	if (effect->flags & effFlagsProgramChunks)
	{
		BYTE* chunk = NULL;
		int size = (int)effect->dispatcher(effect, effGetChunk, 1, 0, &chunk, 0.0f);
		DWORD stringLength;
		CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &stringLength);
		wchar_t* string = new wchar_t[stringLength];
		if (CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, string, &stringLength) == TRUE)
			chunkData = string;
		delete[] string;
	}
	else
	{
		for (int i = 0; i < effect->numParams; i++)
		{
			char buf[256];
			effect->dispatcher(effect, effGetParamName, i, 0, buf, 0.0f);
			float value = effect->getParameter(effect, i);
			paramMap[StringHelper::toWString(buf, CP_UTF8)] = value;
		}
	}
}

void VSTPluginInstance::startProcessing()
{
	if (effect == NULL)
		return;

	effect->dispatcher(effect, effMainsChanged, 0, 1, NULL, 0.0f);
	effect->dispatcher(effect, effStartProcess, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::processReplacing(float** inputArray, float** outputArray, int frameCount)
{
	// TODO: recover
}

void VSTPluginInstance::processDoubleReplacing(double** inputArray, double** outputArray, int frameCount)
{
	// TODO: recover
}

void VSTPluginInstance::process(float** inputArray, float** outputArray, int frameCount)
{
	// TODO: recover
}

void VSTPluginInstance::stopProcessing()
{
	if (effect == NULL)
		return;

	effect->dispatcher(effect, effStopProcess, 0, 0, NULL, 0.0f);
	effect->dispatcher(effect, effMainsChanged, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::startEditing(HWND hWnd, short* width, short* height)
{
	if (effect == NULL)
		return;

	VstRect* rect;
	effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0.0f);
	effect->dispatcher(effect, effEditOpen, 0, 0, hWnd, 0.0f);
	effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0.0f);

	*width = rect->right - rect->left;
	*height = rect->bottom - rect->top;
}

void VSTPluginInstance::doIdle()
{
	if (effect == NULL)
		return;

	effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::stopEditing()
{
	if (effect == NULL)
		return;

	effect->dispatcher(effect, effEditClose, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::setAutomateFunc(std::function<void()> func)
{
	automateFunc = func;
}

void VSTPluginInstance::onAutomate()
{
	if (automateFunc)
		automateFunc();
}

void VSTPluginInstance::setSizeWindowFunc(std::function<void(int, int)> func)
{
	sizeWindowFunc = func;
}

void VSTPluginInstance::onSizeWindow(int w, int h)
{
	if (sizeWindowFunc)
		sizeWindowFunc(w, h);
}
