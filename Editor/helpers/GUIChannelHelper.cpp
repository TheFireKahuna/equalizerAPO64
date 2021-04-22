/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2015  Jonas Thedering

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

#include <Ks.h>
#include <KsMedia.h>

#include "GUIChannelHelper.h"

GUIChannelHelper* GUIChannelHelper::instance = NULL;

GUIChannelHelper* GUIChannelHelper::getInstance()
{
	if (instance == NULL)
		instance = new GUIChannelHelper();

	return instance;
}

GUIChannelHelper::GUIChannelHelper()
{
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_MONO, tr("Mono")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_STEREO, tr("Stereo")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_QUAD, tr("Quadraphonic")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_SURROUND, tr("Surround")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_5POINT1, tr("5.1")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_5POINT1_SURROUND, tr("5.1 Surround")});
	channelConfigurationInfos.append({KSAUDIO_SPEAKER_7POINT1_SURROUND, tr("7.1 Surround")});
}

const QList<GUIChannelHelper::ChannelConfigurationInfo>& GUIChannelHelper::getChannelConfigurationInfos() const
{
	return channelConfigurationInfos;
}
