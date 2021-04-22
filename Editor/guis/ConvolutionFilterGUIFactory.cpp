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

#include "DeviceAPOInfo.h"
#include "Editor/FilterTable.h"
#include "ConvolutionFilterGUI.h"
#include "ConvolutionFilterGUIFactory.h"

using namespace std;

ConvolutionFilterGUIFactory::ConvolutionFilterGUIFactory()
{
}

void ConvolutionFilterGUIFactory::initialize(FilterTable* filterTable)
{
	shared_ptr<AbstractAPOInfo> selectedDevice = filterTable->getSelectedDevice();
	if (selectedDevice != NULL)
		deviceSampleRate = filterTable->getSelectedDevice()->getSampleRate();
	else
		deviceSampleRate = 0;
}

QList<FilterTemplate> ConvolutionFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Convolution (Convolution with impulse response)"), "Convolution:", QStringList(tr("Advanced filters"))));
	return list;
}

void ConvolutionFilterGUIFactory::startOfFile(const QString& configPath)
{
	this->configPath = configPath;
}

IFilterGUI* ConvolutionFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	ConvolutionFilterGUI* result = NULL;

	if (command == "Convolution")
	{
		result = new ConvolutionFilterGUI(configPath, deviceSampleRate, parameters.trimmed());
	}

	return result;
}
