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
#include "helpers/LogHelper.h"
#include "helpers/StringHelper.h"
#include "ReceiveThread.h"

ReceiveThread::ReceiveThread(std::wstring pipeName)
	:pipeName(pipeName)
{
	thread = std::thread(&ReceiveThread::run, this);
}

ReceiveThread::~ReceiveThread()
{
	stop();
}

void ReceiveThread::stop()
{
	if (pipeName != L"")
	{
		HANDLE pipe = CreateFileW((L"\\\\.\\pipe\\" + pipeName).c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (pipe == INVALID_HANDLE_VALUE)
		{
			if (WaitNamedPipeW((L"\\\\.\\pipe\\" + pipeName).c_str(), 1000))
				pipe = CreateFileW((L"\\\\.\\pipe\\" + pipeName).c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		}
		if (pipe != INVALID_HANDLE_VALUE)
		{
			DWORD bytesWritten;
			WriteFile(pipe, "stop", 4, &bytesWritten, NULL);
			FlushFileBuffers(pipe);
			CloseHandle(pipe);
		}
		pipeName = L"";
	}

	if (thread.joinable())
		thread.join();
}

void ReceiveThread::run()
{
	try
	{
		PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (!pSD)
			throw ReceiveException(L"Could not allocate security descriptor: " + StringHelper::getSystemErrorString(GetLastError()));
		SCOPE_EXIT{LocalFree(pSD);};

		if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
			throw ReceiveException(L"Could not initialize security descriptor: " + StringHelper::getSystemErrorString(GetLastError()));

		if (!SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE))
			throw ReceiveException(L"Could not set security descriptor DACL: " + StringHelper::getSystemErrorString(GetLastError()));

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = FALSE;

		char buf[1024];
		while (true)
		{
			HANDLE pipe = CreateNamedPipeW((L"\\\\.\\pipe\\" + pipeName).c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
				PIPE_UNLIMITED_INSTANCES, 0, sizeof(buf), 0, &sa);
			if (pipe == INVALID_HANDLE_VALUE)
				throw ReceiveException(L"Could not create named pipe: " + StringHelper::getSystemErrorString(GetLastError()));
			SCOPE_EXIT{CloseHandle(pipe);};

			bool connected = ConnectNamedPipe(pipe, NULL);
			if (!connected)
				connected = GetLastError() == ERROR_PIPE_CONNECTED;

			if (!connected)
				continue;

			SCOPE_EXIT{DisconnectNamedPipe(pipe);};

			DWORD bytesRead;
			bool ok = ReadFile(pipe, buf, sizeof(buf), &bytesRead, NULL);
			if (!ok || bytesRead == 0)
				throw ReceiveException(L"Could not read from pipe: " + StringHelper::getSystemErrorString(GetLastError()));

			std::string s(buf, bytesRead);
			if (s == "stop")
				break;

			std::scoped_lock lock(mutex);
			answers.push_back(s);
			cond.notify_all();
		}
	}
	catch (ReceiveException e)
	{
		std::scoped_lock lock(mutex);
		caughtException = e;
		cond.notify_all();
	}
	pipeName = L"";
}
