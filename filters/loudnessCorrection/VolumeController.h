/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Alexander Walch

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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <EndpointVolume.h>

class VolumeController
{
public:
	VolumeController();
	HRESULT getVolume(float& currentVolume);
	HRESULT setVolume(float volume);
private:
	IAudioEndpointVolume* _endpointVolume;
	float _minVol;
	float _maxVol;
};
