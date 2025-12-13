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
#include <inttypes.h>
#include "StringHelper.h"
#include "../Version.h"
#include "VSTPluginLibrary.h"
#include "VSTPluginInstance.h"

using namespace std;

#define equalizerApoVSTID VST_FOURCC('E', 'A', 'P', 'O');

vst_time_info vstTime{ 0,0,0,0,0,0,0,0,0,0,{0}, 0xFFFF };

static intptr_t callback(struct vst_effect_t* effect, int32_t opcode, int32_t index, int64_t value, const char* ptr, float opt)
{
	VSTPluginInstance* instance = effect != NULL ? (VSTPluginInstance*)effect->host_internal : NULL;
#ifdef _DEBUG
	printf("vst: %p opcode: %d index: %d value: %" PRIdPTR " ptr: %p opt: %f host_internal: %p\n",
		effect, opcode, index, value, ptr, opt, effect != NULL ? effect->host_internal : NULL);
	fflush(stdout);
#endif

	switch (opcode)
	{
	case VST_HOST_OPCODE_VST_VERSION:
		return VST_VERSION_2_4_0_0;

	case VST_HOST_OPCODE_CURRENT_EFFECT_ID:
		return equalizerApoVSTID;

	case VST_EFFECT_OPCODE_PRODUCT_NAME:
		strcpy_s((char*) ptr, 64, "Equalizer APO");
		return 1;

	case VST_EFFECT_OPCODE_VENDOR_VERSION:
		return (intptr_t) (MAJOR << 24 | MINOR << 16 | REVISION << 8 | 0);

	case VST_HOST_OPCODE_PIN_CONNECTED:
		if (instance != NULL)
			return index < instance->getUsedChannelCount() ? 0 : 1;
		else
			return 0;

	case VST_HOST_OPCODE_IO_NEED_IDLE:
		return effect != NULL ? effect->control(effect, VST_HOST_OPCODE_KEEPALIVE_OR_IDLE, 0, 0, NULL, 0.0f) : 0;

	case VST_HOST_OPCODE_EDITOR_UPDATE:
		return effect != NULL ? effect->control(effect, VST_EFFECT_OPCODE_EDITOR_KEEP_ALIVE, 0, 0, NULL, 0.0f) : 0;

	case VST_HOST_OPCODE_GET_TIME:
		if (instance != NULL) {
			vstTime.sampleRate = instance->getSampleRate();
			return (intptr_t)&vstTime;
		}
		return 0;

	case VST_HOST_OPCODE_GET_SAMPLE_RATE:
		if (instance != NULL)
			return (intptr_t)instance->getSampleRate();
		return 0;

	case VST_HOST_OPCODE_GET_ACTIVE_THREAD:
		if (instance != NULL)
			return instance->getProcessLevel();
		return 0;

	case VST_HOST_OPCODE_LANGUAGE:
		if (instance != NULL)
			return instance->getLanguage();
		return 0;

	case VST_HOST_OPCODE_GET_REPLACE_OR_ACCUMULATE:
		return 1;

	case VST_HOST_OPCODE_EDITOR_RESIZE:
		if (instance != NULL)
		{
			instance->onSizeWindow((int)index, (int)value);
			return 0;
		}
		return 1;

	case VST_HOST_OPCODE_SUPPORTS:
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

	case VST_HOST_OPCODE_AUTOMATE:
		if (instance != NULL)
			instance->onAutomate();
		return 0;

	case VST_HOST_OPCODE_PARAM_START_EDIT:
	case VST_HOST_OPCODE_PARAM_STOP_EDIT:
	case VST_HOST_OPCODE_KEEPALIVE_OR_IDLE:
	case VST_HOST_OPCODE_WANT_MIDI:
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
		effect->control(effect, VST_EFFECT_OPCODE_DESTROY, 0, 0, NULL, 0.0f);
		effect = NULL;
	}
}

bool VSTPluginInstance::initialize()
{
	bool result = true;

	__try
	{
		effect = library->VSTPluginMain(callback);
		effect->host_internal = this;
		if (effect->magic_number == VST_MAGICNUMBER)
		{
			effect->control(effect, VST_EFFECT_OPCODE_INITIALIZE, 0, 0, NULL, 0.0f);

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

	return effect->num_inputs;
}

int VSTPluginInstance::numOutputs() const
{
	if (effect == NULL)
		return 0;

	return effect->num_outputs;
}

bool VSTPluginInstance::canReplacing() const
{
	if (effect == NULL)
		return true;

	return (effect->flags & VST_EFFECT_FLAG_SUPPORTS_FLOAT) != 0;
}

int VSTPluginInstance::uniqueID() const
{
	if (effect == NULL)
		return 0;

	return effect->unique_id;
}

std::wstring VSTPluginInstance::getName() const
{
	if (effect == NULL)
		return L"";

	char buf[256];
	memset(buf, 0, sizeof(buf));
	effect->control(effect, VST_EFFECT_OPCODE_EFFECT_NAME, 0, 0, buf, 0.0f);
	buf[255] = '\0'; // just to be sure

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

bool VSTPluginInstance::canDoubleReplacing() const
{
	return (effect->flags & VST_EFFECT_FLAG_SUPPORTS_DOUBLE) != 0;
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
	effect->control(effect, VST_EFFECT_OPCODE_SET_SAMPLE_RATE, 0, 0, NULL, sampleRate);
	effect->control(effect, VST_EFFECT_OPCODE_SET_BLOCK_SIZE, 0, blockSize, NULL, 0.0f);
}

void VSTPluginInstance::writeToEffect(const std::wstring& chunkData, const std::unordered_map<std::wstring, float>& paramMap)
{
	if (effect == NULL)
		return;

	if (effect->flags & VST_EFFECT_FLAG_CHUNKS)
	{
		if (chunkData != L"")
		{
			DWORD bufSize = 0;
			CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, NULL, &bufSize, NULL, NULL);
			BYTE* buf = new BYTE[bufSize];
			if (CryptStringToBinaryW(chunkData.c_str(), 0, CRYPT_STRING_BASE64, buf, &bufSize, NULL, NULL) == TRUE)
				effect->control(effect, VST_EFFECT_OPCODE_SET_CHUNK_DATA, 1, bufSize, buf, 0.0f);
			delete[] buf;
		}
	}
	else
	{
		for (int i = 0; i < effect->num_params; i++)
		{
			char buf[256];
			effect->control(effect, VST_EFFECT_OPCODE_PARAM_GET_NAME, i, 0, buf, 0.0f);
			buf[255] = '\0'; // just to be sure
			wstring name = StringHelper::toWString(buf, CP_UTF8);
			auto it = paramMap.find(name);
			if (it != paramMap.end())
				effect->set_parameter(effect, i, it->second);
		}
	}
}

void VSTPluginInstance::readFromEffect(std::wstring& chunkData, std::unordered_map<std::wstring, float>& paramMap) const
{
	if (effect == NULL)
		return;

	chunkData = L"";
	paramMap.clear();

	if (effect->flags & VST_EFFECT_FLAG_CHUNKS)
	{
		BYTE* chunk = NULL;
		int size = (int)effect->control(effect, VST_EFFECT_OPCODE_GET_CHUNK_DATA, 1, 0, &chunk, 0.0f);
		DWORD stringLength = 0;
		CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &stringLength);
		wchar_t* string = new wchar_t[stringLength];
		if (CryptBinaryToStringW(chunk, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, string, &stringLength) == TRUE)
			chunkData = string;
		delete[] string;
	}
	else
	{
		for (int i = 0; i < effect->num_params; i++)
		{
			char buf[256];
			effect->control(effect, VST_EFFECT_OPCODE_PARAM_GET_NAME, i, 0, buf, 0.0f);
			buf[255] = '\0'; // just to be sure
			float value = effect->get_parameter(effect, i);
			paramMap[StringHelper::toWString(buf, CP_UTF8)] = value;
		}
	}
}

void VSTPluginInstance::startProcessing()
{
	if (effect == NULL)
		return;

	effect->control(effect, VST_EFFECT_OPCODE_SUSPEND, 0, 1, NULL, 0.0f);
	effect->control(effect, VST_EFFECT_OPCODE_PROCESS_BEGIN, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::processReplacing(float** inputArray, float** outputArray, int frameCount)
{
	if (effect == NULL)
		return;

	effect->process_float(effect, inputArray, outputArray, frameCount);
}

void VSTPluginInstance::processDoubleReplacing(double** inputArray, double** outputArray, int frameCount)
{
	if (effect == NULL)
		return;

	effect->process_double(effect, inputArray, outputArray, frameCount);
}

void VSTPluginInstance::process(float** inputArray, float** outputArray, int frameCount)
{
	if (effect == NULL)
		return;

	effect->process(effect, inputArray, outputArray, frameCount);
}

void VSTPluginInstance::stopProcessing()
{
	if (effect == NULL)
		return;

	effect->control(effect, VST_EFFECT_OPCODE_PROCESS_END, 0, 0, NULL, 0.0f);
	effect->control(effect, VST_EFFECT_OPCODE_SUSPEND, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::startEditing(HWND hWnd, short* width, short* height)
{
	if (effect == NULL)
		return;

	vst_rect_t* rect;
	effect->control(effect, VST_EFFECT_OPCODE_EDITOR_GET_RECT, 0, 0, &rect, 0.0f);
	effect->control(effect, VST_EFFECT_OPCODE_EDITOR_OPEN, 0, 0, hWnd, 0.0f);
	effect->control(effect, VST_EFFECT_OPCODE_EDITOR_GET_RECT, 0, 0, &rect, 0.0f);

	*width = rect->right - rect->left;
	*height = rect->bottom - rect->top;
}

void VSTPluginInstance::doIdle()
{
	if (effect == NULL)
		return;

	effect->control(effect, VST_EFFECT_OPCODE_EDITOR_KEEP_ALIVE, 0, 0, NULL, 0.0f);
}

void VSTPluginInstance::stopEditing()
{
	if (effect == NULL)
		return;

	effect->control(effect, VST_EFFECT_OPCODE_EDITOR_CLOSE, 0, 0, NULL, 0.0f);
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
