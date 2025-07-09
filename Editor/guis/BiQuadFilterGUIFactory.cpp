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

#include "BiQuadFilterGUI.h"
#include "BiQuadFilterGUIFactory.h"
#include <filters/BiQuadFilterFactory.h>

BiQuadFilterGUIFactory::BiQuadFilterGUIFactory()
{
}

QList<FilterTemplate> BiQuadFilterGUIFactory::createFilterTemplates()
{
	QStringList path(tr("Parametric filters"));

	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Peaking filter"), "Filter: ON PK Fc 100 Hz Gain 0 dB Q 10", path));
	list.append(FilterTemplate(tr("Low-pass filter"), "Filter: ON LP Fc 100 Hz", path));
	list.append(FilterTemplate(tr("High-pass filter"), "Filter: ON HP Fc 100 Hz", path));
	list.append(FilterTemplate(tr("Band-pass filter"), "Filter: ON BP Fc 100 Hz Q 10", path));
	list.append(FilterTemplate(tr("Low-shelf filter"), "Filter: ON LS Fc 100 Hz Gain 0 dB", path));
	list.append(FilterTemplate(tr("High-shelf filter"), "Filter: ON HS Fc 100 Hz Gain 0 dB", path));
	list.append(FilterTemplate(tr("Notch filter"), "Filter: ON NO Fc 100 Hz", path));
	list.append(FilterTemplate(tr("All-pass filter"), "Filter: ON AP Fc 100 Hz Q 10", path));
	return list;
}

IFilterGUI* BiQuadFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	BiQuadFilterGUI* result = NULL;

	if (command.startsWith("Filter"))
	{
		BiQuadFilterFactory factory;
		std::wstring commandWStr = command.toStdWString();
		std::wstring paramWStr = parameters.toStdWString();
		std::vector<IFilter*> filters = factory.createFilter(L"", commandWStr, paramWStr);
		if (!filters.empty())
		{
			BiQuadFilter* filter = (BiQuadFilter*)filters[0];
			result = new BiQuadFilterGUI(filter);
		}

		for (IFilter* f : filters)
		{
			f->~IFilter();
			MemoryHelper::free(f);
		}
	}

	return result;
}
