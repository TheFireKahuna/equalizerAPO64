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
#include <fstream>
#include <sstream>
#include <ObjBase.h>
#include <aclapi.h>
#include <authz.h>

#include "StringHelper.h"
#include "RegistryHelper.h"

using namespace std;

DWORD RegistryHelper::windowsVersion = 0;

wstring RegistryHelper::readValue(wstring key, wstring valuename)
{
	wstring result;

	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	LSTATUS status;
	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	if (type != REG_SZ)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
	}

	wchar_t* buf = new wchar_t[bufSize / sizeof(wchar_t) + 1];
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, (LPBYTE)buf, &bufSize);

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
	{
		delete buf;
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	// Remove zero-termination
	if (buf[bufSize / sizeof(wchar_t) - 1] == L'\0')
		bufSize -= sizeof(wchar_t);
	result = wstring((wchar_t*)buf, (wstring::size_type)bufSize / sizeof(wchar_t));
	delete buf;

	return result;
}

unsigned long RegistryHelper::readDWORDValue(wstring key, wstring valuename)
{
	unsigned long result;

	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	LSTATUS status;
	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	if (type != REG_DWORD)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
	}

	BYTE* buf = new BYTE[bufSize];
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, buf, &bufSize);

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
	{
		delete buf;
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	result = ((unsigned long*)buf)[0];
	delete buf;

	return result;
}

vector<wstring> RegistryHelper::readMultiValue(wstring key, wstring valuename)
{
	vector<wstring> result;

	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	LSTATUS status;
	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	if (type != REG_MULTI_SZ)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
	}

	wchar_t* buf = new wchar_t[bufSize / sizeof(wchar_t) + 1];
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, (LPBYTE)buf, &bufSize);

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
	{
		delete buf;
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	size_t length = bufSize / sizeof(wchar_t);
	// Remove zero-termination
	while (length > 0 && buf[length - 1] == L'\0')
		length--;

	size_t start = 0;
	for (size_t i = 0; i < length; i++)
	{
		if (buf[i] == L'\0')
		{
			result.push_back(wstring(buf + start, i - start));
			start = i + 1;
		}
	}

	if (length > start)
		result.push_back(wstring(buf + start, length - start));

	delete buf;

	return result;
}

vector<unsigned char> RegistryHelper::readBinaryValue(wstring key, wstring valuename)
{
	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	LSTATUS status;
	DWORD type;
	DWORD bufSize;
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	if (status != ERROR_SUCCESS)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	if (type != REG_BINARY)
	{
		RegCloseKey(keyHandle);
		throw RegistryException(L"Registry value " + key + L"\\" + valuename + L" has wrong type");
	}

	vector<unsigned char> result(bufSize, 0);
	status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, NULL, result.data(), &bufSize);

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
	{
		throw RegistryException(L"Error while reading registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
	}

	return result;
}

void RegistryHelper::writeValue(wstring key, wstring valuename, wstring value)
{
	HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);

	LSTATUS status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)((value.size() + 1) * sizeof(wchar_t)));

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while writing to registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::writeDWORDValue(wstring key, wstring valuename, unsigned long value)
{
	HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);

	LSTATUS status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_DWORD, (const BYTE*)&value, sizeof(unsigned long));

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while writing to registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::writeMultiValue(wstring key, wstring valuename, wstring value)
{
	HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);

	wchar_t* data = new wchar_t[value.size() + 2];
	value._Copy_s(data, (value.size() + 2) * sizeof(wchar_t), value.size());
	data[value.size()] = L'\0';
	data[value.size() + 1] = L'\0';

	LSTATUS status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_MULTI_SZ, (const BYTE*)data, (DWORD)((value.size() + 2) * sizeof(wchar_t)));

	delete data;

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while writing to registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::writeMultiValue(wstring key, wstring valuename, vector<wstring> values)
{
	HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);

	size_t size = 1;
	for (wstring value : values)
		size += value.size() + 1;

	wchar_t* data = new wchar_t[size];
	size_t offset = 0;
	for (wstring value : values)
	{
		value._Copy_s(data + offset, size * sizeof(wchar_t), value.size());
		offset += value.size();
		data[offset++] = L'\0';
	}
	data[offset] = L'\0';

	LSTATUS status = RegSetValueExW(keyHandle, valuename.c_str(), 0, REG_MULTI_SZ, (const BYTE*)data, (DWORD)(size * sizeof(wchar_t)));

	delete data;

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while writing to registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::deleteValue(wstring key, wstring valuename)
{
	HKEY keyHandle = openKey(key, KEY_SET_VALUE | KEY_WOW64_64KEY);

	LSTATUS status = RegDeleteValueW(keyHandle, valuename.c_str());

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while deleting registry value " + key + L"\\" + valuename + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::createKey(wstring key)
{
	HKEY rootKey;
	wstring subKey = splitKey(key, &rootKey);

	HKEY keyHandle;
	LSTATUS status = RegCreateKeyExW(rootKey, subKey.c_str(), 0, NULL, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &keyHandle, NULL);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while creating registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	RegCloseKey(keyHandle);
}

void RegistryHelper::deleteKey(wstring key)
{
	HKEY rootKey;
	wstring subKey = splitKey(key, &rootKey);

	LSTATUS status = RegDeleteKeyExW(rootKey, subKey.c_str(), KEY_WOW64_64KEY, 0);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while deleting registry key " + key + L": " + StringHelper::getSystemErrorString(status));
}

void RegistryHelper::makeWritable(wstring key)
{
	HKEY keyHandle = openKey(key, READ_CONTROL | WRITE_DAC | KEY_WOW64_64KEY);

	DWORD descriptorSize = 0;
	RegGetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, NULL, &descriptorSize);

	PSECURITY_DESCRIPTOR oldSd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, descriptorSize);
	LSTATUS status = RegGetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, oldSd, &descriptorSize);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while getting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	BOOL aclPresent, aclDefaulted;
	PACL oldAcl = NULL;
	if (!GetSecurityDescriptorDacl(oldSd, &aclPresent, &oldAcl, &aclDefaulted))
		throw RegistryException(L"Error in GetSecurityDescriptorDacl while ensuring writability");

	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &sid))
		throw RegistryException(L"Error in AllocateAndInitializeSid while ensuring writability");

	EXPLICIT_ACCESS ea;
	ea.grfAccessPermissions = KEY_ALL_ACCESS;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea.Trustee.ptstrName = (LPWSTR)sid;

	PACL acl = NULL;
	if (ERROR_SUCCESS != SetEntriesInAcl(1, &ea, oldAcl, &acl))
		throw RegistryException(L"Error in SetEntriesInAcl while ensuring writability");

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == sd)
		throw RegistryException(L"Error in LocalAlloc while ensuring writability");

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
		throw RegistryException(L"Error in InitializeSecurityDescriptor while ensuring writability");

	if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
		throw RegistryException(L"Error in SetSecurityDescriptorDacl while ensuring writability");

	status = RegSetKeySecurity(keyHandle, DACL_SECURITY_INFORMATION, sd);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while setting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	FreeSid(sid);
	LocalFree(acl);
	HeapFree(GetProcessHeap(), 0, oldSd);
	LocalFree(sd);
}

void RegistryHelper::takeOwnership(wstring key)
{
	HANDLE tokenHandle;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
		throw RegistryException(L"Error in OpenProcessToken while taking ownership");

	LUID luid;
	if (!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &luid))
		throw RegistryException(L"Error in LookupPrivilegeValue while taking ownership");

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

	HKEY keyHandle = openKey(key, WRITE_OWNER | KEY_WOW64_64KEY);

	PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (NULL == sd)
		throw RegistryException(L"Error in SetPrivilege while taking ownership");

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
		throw RegistryException(L"Error in InitializeSecurityDescriptor while taking ownership");

	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &sid))
		throw RegistryException(L"Error in AllocateAndInitializeSid while taking ownership");

	if (!SetSecurityDescriptorOwner(sd, sid, FALSE))
		throw RegistryException(L"Error in SetSecurityDescriptorOwner while taking ownership");

	LSTATUS status = RegSetKeySecurity(keyHandle, OWNER_SECURITY_INFORMATION, sd);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while setting security information for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(tokenHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		throw RegistryException(L"Error in AdjustTokenPrivileges while taking ownership");

	FreeSid(sid);
	LocalFree(sd);
}

ACCESS_MASK RegistryHelper::getFileAccessForUser(std::wstring path, unsigned long rid)
{
	ACCESS_MASK result;

	PSECURITY_DESCRIPTOR sd;
	if (GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION
		| GROUP_SECURITY_INFORMATION, NULL, NULL, NULL, NULL, &sd) != ERROR_SUCCESS)
		throw RegistryException(L"Error in GetNamedSecurityInfoW while getting file access");

	AUTHZ_RESOURCE_MANAGER_HANDLE manager;
	if (!AuthzInitializeResourceManager(AUTHZ_RM_FLAG_NO_AUDIT, NULL, NULL, NULL, NULL, &manager))
		throw RegistryException(L"Error in AuthzInitializeResourceManager while getting file access");

	PSID sid = NULL;
	SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
	if (!AllocateAndInitializeSid(&authority, 1, rid, 0, 0, 0, 0, 0, 0, 0, &sid))
		throw RegistryException(L"Error in AllocateAndInitializeSid while getting file access");

	LUID unusedId = {0};
	AUTHZ_CLIENT_CONTEXT_HANDLE context;
	if (!AuthzInitializeContextFromSid(0, sid, manager, NULL, unusedId, NULL, &context))
		throw RegistryException(L"Error in AuthzInitializeContextFromSid while getting file access");

	AUTHZ_ACCESS_REQUEST request = {0};

	request.DesiredAccess = MAXIMUM_ALLOWED;
	request.PrincipalSelfSid = NULL;
	request.ObjectTypeList = NULL;
	request.ObjectTypeListLength = 0;
	request.OptionalArguments = NULL;

	AUTHZ_ACCESS_REPLY reply = {0};
	BYTE buf[1024];
	RtlZeroMemory(buf, sizeof(buf));
	reply.ResultListLength = 1;
	reply.GrantedAccessMask = (ACCESS_MASK*)buf;
	reply.Error = (DWORD*)(buf + sizeof(ACCESS_MASK));

	if (!AuthzAccessCheck(0, context, &request, NULL, sd, NULL, 0, &reply, NULL))
		throw RegistryException(L"Error in AuthzAccessCheck while getting file access");

	result = *reply.GrantedAccessMask;

	AuthzFreeContext(context);
	FreeSid(sid);
	AuthzFreeResourceManager(manager);
	LocalFree(sd);

	return result;
}

bool RegistryHelper::keyExists(wstring key)
{
	bool result;

	HKEY rootKey;
	wstring subKey = splitKey(key, &rootKey);

	HKEY keyHandle;
	result = (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &keyHandle) == ERROR_SUCCESS);

	if (result)
		RegCloseKey(keyHandle);

	return result;
}

bool RegistryHelper::valueExists(wstring key, wstring valuename)
{
	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	DWORD type;
	DWORD bufSize;
	LSTATUS status = RegQueryValueExW(keyHandle, valuename.c_str(), NULL, &type, NULL, &bufSize);
	RegCloseKey(keyHandle);
	return status == ERROR_SUCCESS;
}

vector<wstring> RegistryHelper::enumSubKeys(wstring key)
{
	vector<wstring> result;

	HKEY keyHandle = openKey(key, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY);

	wchar_t keyName[256];
	DWORD keyLength = sizeof(keyName) / sizeof(wchar_t);
	int i = 0;

	LSTATUS status;
	while ((status = RegEnumKeyExW(keyHandle, i++, keyName, &keyLength, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
	{
		keyLength = sizeof(keyName) / sizeof(wchar_t);

		result.push_back(keyName);
	}

	RegCloseKey(keyHandle);

	if (status != ERROR_NO_MORE_ITEMS)
		throw RegistryException(L"Error while enumerating sub keys of registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	return result;
}

bool RegistryHelper::keyEmpty(wstring key)
{
	HKEY keyHandle = openKey(key, KEY_QUERY_VALUE | KEY_WOW64_64KEY);

	DWORD keyCount;
	DWORD valueCount;
	LSTATUS status = RegQueryInfoKeyW(keyHandle, NULL, NULL, NULL, &keyCount, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);

	RegCloseKey(keyHandle);

	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while reading info for registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	return keyCount == 0 && valueCount == 0;
}

void RegistryHelper::saveToFile(wstring key, vector<wstring> valuenames, wstring filepath)
{
	wofstream stream(filepath);
	if (!stream.good())
		throw RegistryException(L"Error while opening file " + filepath + L" for writing");

	stream << L"Windows Registry Editor Version 5.00\n" << endl;
	stream << L"[HKEY_LOCAL_MACHINE\\" << key << L"]" << endl;
	for (vector<wstring>::iterator it = valuenames.begin(); it != valuenames.end(); it++)
	{
		wstring& valuename = *it;
		wstring value = readValue(key, valuename);

		stream << L"\"" << valuename << L"\"=\"" << value << L"\"" << endl;
	}
	stream << endl;

	stream.close();
}

wstring RegistryHelper::getGuidString(GUID guid)
{
	wchar_t* temp;
	if (FAILED(StringFromCLSID(guid, &temp)))
		throw RegistryException(L"Could not convert GUID to string");
	std::wstring result(temp);
	CoTaskMemFree(temp);

	return result;
}

bool RegistryHelper::isWindowsVersionAtLeast(unsigned major, unsigned minor)
{
	if (windowsVersion == 0)
	{
		DWORD handle;
		DWORD size = GetFileVersionInfoSizeW(L"kernel32.dll", &handle);
		if (size != 0)
		{
			void* data = malloc(size);
			if (GetFileVersionInfo(L"kernel32.dll", handle, size, data))
			{
				VS_FIXEDFILEINFO* info;
				UINT len;
				if (VerQueryValueW(data, L"\\", (LPVOID*)&info, &len))
					windowsVersion = info->dwProductVersionMS;
			}
			free(data);
		}
	}

	// this will only work for major and minor up to 99
	DWORD compareVersion = ((major / 10) << 20) + ((major % 10) << 16) + ((minor / 10) << 4) + (minor % 10);

	return windowsVersion >= compareVersion;
}

HKEY RegistryHelper::openKey(const wstring& key, REGSAM samDesired)
{
	HKEY rootKey;
	wstring subKey = splitKey(key, &rootKey);

	HKEY keyHandle;
	LSTATUS status = RegOpenKeyExW(rootKey, subKey.c_str(), 0, samDesired, &keyHandle);
	if (status != ERROR_SUCCESS)
		throw RegistryException(L"Error while opening registry key " + key + L": " + StringHelper::getSystemErrorString(status));

	return keyHandle;
}

wstring RegistryHelper::splitKey(const wstring& key, HKEY* rootKey)
{
	size_t pos = key.find(L'\\');
	if (pos == wstring::npos)
		throw RegistryException(L"Key " + key + L" has invalid format");

	wstring rootPart = key.substr(0, pos);
	wstring pathPart = key.substr(pos + 1);

	wstring p = StringHelper::toUpperCase(rootPart);
	if (p == L"HKEY_CLASSES_ROOT")
		*rootKey = HKEY_CLASSES_ROOT;
	else if (p == L"HKEY_CURRENT_CONFIG")
		*rootKey = HKEY_CURRENT_CONFIG;
	else if (p == L"HKEY_CURRENT_USER")
		*rootKey = HKEY_CURRENT_USER;
	else if (p == L"HKEY_LOCAL_MACHINE")
		*rootKey = HKEY_LOCAL_MACHINE;
	else if (p == L"HKEY_USERS")
		*rootKey = HKEY_USERS;
	else
		throw RegistryException(L"Unknown root key " + rootPart);

	return pathPart;
}
