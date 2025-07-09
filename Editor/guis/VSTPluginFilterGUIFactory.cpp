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

#include "helpers/VSTPluginLibrary.h"
#include "helpers/StringHelper.h"
#include "filters/VSTPluginFilterFactory.h"
#include "filters/VSTPluginFilter.h"
#include "VSTPluginFilterGUI.h"
#include "VSTPluginFilterGUIFactory.h"

using namespace std;

QList<FilterTemplate> VSTPluginFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("VST plugin"), "VSTPlugin:", QStringList(tr("Plugins"))));
	return list;
}

IFilterGUI* VSTPluginFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	VSTPluginFilterGUI* result = NULL;

	if (command == "VSTPlugin")
	{
		VSTPluginFilterFactory factory;
		std::wstring commandWStr = command.toStdWString();
		std::wstring paramWStr = parameters.toStdWString();
		std::vector<IFilter*> filters = factory.createFilter(L"", commandWStr, paramWStr);
		if (!filters.empty())
		{
			VSTPluginFilter* filter = (VSTPluginFilter*)filters[0];
			result = new VSTPluginFilterGUI(filter->getLibrary(), filter->getChunkData(), filter->getParamMap());
		}
		else
		{
			result = new VSTPluginFilterGUI(VSTPluginLibrary::getInstance(L""), L"", unordered_map<wstring, float>());
		}

		for (IFilter* f : filters)
		{
			f->~IFilter();
			MemoryHelper::free(f);
		}
	}

	return result;
}
