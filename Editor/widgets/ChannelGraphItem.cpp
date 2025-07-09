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

#include <QGraphicsScene>
#include <QPainter>

#include "Editor/helpers/GUIHelper.h"
#include "ChannelGraphItem.h"

static const float margin = 5;

ChannelGraphItem::ChannelGraphItem(const QString& name)
	: name(name)
{
}

QRectF ChannelGraphItem::boundingRect() const
{
	if (cachedRect.isNull())
	{
		QFontMetrics metrics(scene()->font());
		int width = metrics.size(0, name).width();
		// we only use upper case characters and digits, so everything is above the baseline
		int height = metrics.ascent();
		// make it at least quadratic
		if (width < height)
			width = height;
		// shift by 0.5 to have sharp lines
		cachedRect.setLeft(0.5);
		cachedRect.setTop(0.5);
		cachedRect.setWidth(width + 2 * GUIHelper::scale(margin));
		cachedRect.setHeight(height + 2 * GUIHelper::scale(margin));
	}

	return cachedRect;
}

QString ChannelGraphItem::getName() const
{
	return name;
}

void ChannelGraphItem::paint(QPainter* painter, QColor color)
{
	QRectF rect = boundingRect();
	QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
	gradient.setColorAt(0, QColor(255, 255, 255));
	gradient.setColorAt(1, color);
	painter->setBrush(gradient);
	// make sure that text is visible in dark mode
	if(painter->pen().color() == Qt::white)
		painter->setPen(Qt::black);
	painter->drawRoundedRect(rect, GUIHelper::scale(margin), GUIHelper::scale(margin));
	painter->drawText(rect.translated(1, -1), Qt::AlignCenter, name);
}
