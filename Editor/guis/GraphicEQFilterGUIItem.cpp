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

#include <algorithm>
#include <QPainter>

#include "helpers/SafetyLimits.h"
#include "Editor/helpers/GUIHelper.h"
#include "GraphicEQFilterGUIScene.h"
#include "GraphicEQFilterGUIItem.h"

static double size = 10;

GraphicEQFilterGUIItem::GraphicEQFilterGUIItem(int index, double hz, double db)
	: FrequencyPlotItem(hz, db), index(index)
{
	setFlag(ItemIsMovable);
	setFlag(ItemIsSelectable);
}

QRectF GraphicEQFilterGUIItem::boundingRect() const
{
	int s = GUIHelper::scale(size);
	return QRectF(-s / 2, -s / 2, s, s);
}

QPainterPath GraphicEQFilterGUIItem::shape() const
{
	QPainterPath path;
	path.addEllipse(boundingRect());
	return path;
}

void GraphicEQFilterGUIItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    bool dark = GUIHelper::isDarkMode();
    painter->setBrush(dark ? Qt::black : Qt::white);
	if (isSelected())
		painter->setBrush(QColor(38, 147, 255));
	painter->drawPath(shape());
	if (index < 99)
	{
		if (isSelected())
            painter->setPen(Qt::white);

		QFont font;
		font.setPixelSize(GUIHelper::scale(9));
		font.setLetterSpacing(QFont::AbsoluteSpacing, -1);
		QFontMetrics metrics(font);
		painter->setFont(font);
		QString text = QString::number(index + 1);
		painter->drawText(-metrics.size(0, text).width() / 2, metrics.boundingRect('0').height() / 2, text);
	}
}

QVariant GraphicEQFilterGUIItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
	QVariant result = QGraphicsItem::itemChange(change, value);

	if (change == ItemScenePositionHasChanged)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
		s->itemMoved(index);
	}
	else if (change == ItemSelectedHasChanged)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
		s->itemSelectionChanged(index, value.toBool());
	}
	else if (change == ItemPositionChange)
	{
		GraphicEQFilterGUIScene* s = qobject_cast<GraphicEQFilterGUIScene*>(scene());
		if (s != NULL)
		{
			QPointF newPos = value.toPointF();
			double yTop = s->dbToY(SafetyLimits::MAX_GAIN_DB);
			double yBot = s->dbToY(SafetyLimits::MIN_GAIN_DB);
			double minY = std::min(yTop, yBot);
			double maxY = std::max(yTop, yBot);
			newPos.setY(std::clamp(newPos.y(), minY, maxY));

			if (s->getBandCount() != -1)
			{
				newPos.setX(s->hzToX(getHz()));
			}
			else
			{
				double xLeft = s->hzToX(SafetyLimits::MIN_FREQ_HZ);
				double xRight = s->hzToX(SafetyLimits::MAX_FREQ_HZ);
				double minX = std::min(xLeft, xRight);
				double maxX = std::max(xLeft, xRight);
				newPos.setX(std::clamp(newPos.x(), minX, maxX));
			}
			result = newPos;
		}
	}

	return result;
}
int GraphicEQFilterGUIItem::getIndex() const
{
	return index;
}

void GraphicEQFilterGUIItem::setIndex(int value)
{
	index = value;
}
