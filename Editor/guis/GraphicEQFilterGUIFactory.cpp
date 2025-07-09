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

#include "filters/GraphicEQFilter.h"
#include "filters/GraphicEQFilterFactory.h"
#include "GraphicEQFilterGUI.h"
#include "GraphicEQFilterGUIFactory.h"

GraphicEQFilterGUIFactory::GraphicEQFilterGUIFactory()
{
}

void GraphicEQFilterGUIFactory::initialize(FilterTable* filterTable)
{
	this->filterTable = filterTable;
}

QList<FilterTemplate> GraphicEQFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("15-band graphic equalizer"), "GraphicEQ: 25 0; 40 0; 63 0; 100 0; 160 0; 250 0; 400 0; 630 0; 1000 0; 1600 0; 2500 0; 4000 0; 6300 0; 10000 0; 16000 0", QStringList(tr("Graphic equalizers"))));
	list.append(FilterTemplate(tr("31-band graphic equalizer"), "GraphicEQ: 20 0; 25 0; 31.5 0; 40 0; 50 0; 63 0; 80 0; 100 0; 125 0; 160 0; 200 0; 250 0; 315 0; 400 0; 500 0; 630 0; 800 0; 1000 0; 1250 0; 1600 0; 2000 0; 2500 0; 3150 0; 4000 0; 5000 0; 6300 0; 8000 0; 10000 0; 12500 0; 16000 0; 20000 0", QStringList(tr("Graphic equalizers"))));
	list.append(FilterTemplate(tr("Graphic equalizer with variable bands"), "GraphicEQ: ", QStringList(tr("Graphic equalizers"))));
	return list;
}

void GraphicEQFilterGUIFactory::startOfFile(const QString& configPath)
{
	this->configPath = configPath;
}

IFilterGUI* GraphicEQFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	GraphicEQFilterGUI* result = NULL;

	if (command == "GraphicEQ")
	{
		GraphicEQFilterFactory factory;
		std::wstring commandWStr = command.toStdWString();
		std::wstring paramWStr = parameters.toStdWString();
		std::vector<IFilter*> filters = factory.createFilter(L"", commandWStr, paramWStr);
		if (!filters.empty())
		{
			GraphicEQFilter* filter = (GraphicEQFilter*)filters[0];
			result = new GraphicEQFilterGUI(filter, configPath, filterTable);
		}

		for (IFilter* f : filters)
		{
			f->~IFilter();
			MemoryHelper::free(f);
		}
	}

	return result;
}
