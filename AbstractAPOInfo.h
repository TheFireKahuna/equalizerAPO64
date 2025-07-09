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

#pragma once

#include <string>

class AbstractAPOInfo
{
public:
	virtual ~AbstractAPOInfo();
	virtual std::wstring getConnectionName() const = 0;
	virtual std::wstring getDeviceName() const = 0;
	virtual std::wstring getDeviceGuid() const = 0;
	virtual std::wstring getDeviceString() const = 0;
	virtual unsigned getChannelCount() const = 0;
	virtual unsigned getSampleRate() const = 0;
	virtual unsigned long getChannelMask() const = 0;
	virtual bool isInput() const = 0;
	virtual bool isInstalled() const = 0;
	virtual bool canBeUpgraded() const = 0;
	virtual bool hasChanges() const = 0;
	virtual bool isExperimental() const = 0;
	virtual bool isEnhancementsDisabled() const = 0;
	virtual bool isDefaultDevice() const = 0;
	virtual bool isDisabled() const = 0;
	virtual bool isUnplugged() const = 0;
	virtual void install() = 0;
	virtual void uninstall() = 0;
	virtual void reinstall() = 0;
};
