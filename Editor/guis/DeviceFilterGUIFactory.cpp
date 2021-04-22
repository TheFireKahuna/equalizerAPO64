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

#include "Editor/FilterTable.h"
#include "DeviceFilterGUI.h"
#include "DeviceFilterGUIFactory.h"

using namespace std;

DeviceFilterGUIFactory::DeviceFilterGUIFactory()
{
}

void DeviceFilterGUIFactory::initialize(FilterTable* filterTable)
{
	devices.append(filterTable->getOutputDevices());
	devices.append(filterTable->getInputDevices());
}

QList<FilterTemplate> DeviceFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Device (Select device)"), "Device: all", QStringList(tr("Control"))));
	return list;
}

IFilterGUI* DeviceFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	DeviceFilterGUI* result = NULL;

	if (command == "Device")
	{
		result = new DeviceFilterGUI(this);
		result->load(parameters);
	}

	return result;
}

const QList<shared_ptr<AbstractAPOInfo> >& DeviceFilterGUIFactory::getDevices() const
{
	return devices;
}
