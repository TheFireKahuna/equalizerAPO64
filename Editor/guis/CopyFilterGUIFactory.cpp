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

#include "filters/CopyFilter.h"
#include "filters/CopyFilterFactory.h"
#include "CopyFilterGUI.h"
#include "CopyFilterGUIFactory.h"

CopyFilterGUIFactory::CopyFilterGUIFactory()
{
}

void CopyFilterGUIFactory::initialize(FilterTable* filterTable)
{
	this->filterTable = filterTable;
}

QList<FilterTemplate> CopyFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Copy (Copy between channels)"), "Copy: ", QStringList(tr("Basic filters"))));
	return list;
}

IFilterGUI* CopyFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	CopyFilterGUI* result = NULL;

	if (command == "Copy")
	{
		CopyFilterFactory factory;
		std::vector<IFilter*> filters = factory.createFilter(L"", command.toStdWString(), parameters.toStdWString());
		if (!filters.empty())
		{
			CopyFilter* filter = (CopyFilter*)filters[0];
			result = new CopyFilterGUI(filter, filterTable);
		}

		for (IFilter* f : filters)
		{
			f->~IFilter();
			MemoryHelper::free(f);
		}
	}

	return result;
}
