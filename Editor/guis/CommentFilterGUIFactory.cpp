/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2014  Jonas Thedering

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

#include "helpers/StringHelper.h"
#include "CommentFilterGUI.h"
#include "CommentFilterGUIFactory.h"

using namespace std;

CommentFilterGUIFactory::CommentFilterGUIFactory()
{
}

QList<FilterTemplate> CommentFilterGUIFactory::createFilterTemplates()
{
	QList<FilterTemplate> list;
	list.append(FilterTemplate(tr("Comment"), "# ", QStringList()));
	return list;
}

IFilterGUI* CommentFilterGUIFactory::createFilterGUI(QString& command, QString& parameters)
{
	lastWasComment = command.length() > 0 && command[0] == '#';
	if (lastWasComment)
		command = command.mid(1).trimmed();

	return NULL;
}

IFilterGUI* CommentFilterGUIFactory::decorateFilterGUI(IFilterGUI* gui)
{
	gui = new CommentFilterGUI(gui, lastWasComment);

	return gui;
}
