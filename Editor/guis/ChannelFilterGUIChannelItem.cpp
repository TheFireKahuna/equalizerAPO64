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

#include <QPainter>

#include "ChannelFilterGUIScene.h"
#include "ChannelFilterGUIChannelItem.h"

ChannelFilterGUIChannelItem::ChannelFilterGUIChannelItem(const QString& name)
	: ChannelGraphItem(name)
{
	setFlag(ItemIsSelectable);
}

void ChannelFilterGUIChannelItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QColor color;

	if (isSelected())
	{
		color = QColor(176, 229, 255);
	}
	else
	{
		painter->setPen(QColor(156, 156, 156));
		color = QColor(204, 204, 204);
	}

	ChannelGraphItem::paint(painter, color);
}
