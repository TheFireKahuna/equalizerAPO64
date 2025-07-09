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

#include "filters/DelayFilter.h"
#include "filters/DelayFilterFactory.h"
#include "DelayFilterGUI.h"
#include "DelayFilterGUIFactory.h"

DelayFilterGUIFactory::DelayFilterGUIFactory()
{
}

QList<FilterTemplate> DelayFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Delay"), "Delay: 0 ms", QStringList(tr("Basic filters"))));
	return list;
}

IFilterGUI* DelayFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	DelayFilterGUI* result = NULL;

	if (command == "Delay")
	{
		DelayFilterFactory factory;
		std::wstring commandWStr = command.toStdWString();
		std::wstring paramWStr = parameters.toStdWString();
		std::vector<IFilter*> filters = factory.createFilter(L"", commandWStr, paramWStr);
		if (!filters.empty())
		{
			DelayFilter* filter = (DelayFilter*)filters[0];
			result = new DelayFilterGUI(filter->getDelay(), filter->getIsMs());
		}

		for (IFilter* f : filters)
		{
			f->~IFilter();
			MemoryHelper::free(f);
		}
	}

	return result;
}
