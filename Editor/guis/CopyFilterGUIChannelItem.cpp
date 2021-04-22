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

#include "CopyFilterGUIScene.h"
#include "CopyFilterGUIChannelItem.h"

CopyFilterGUIChannelItem::CopyFilterGUIChannelItem(const QString& name, bool output)
	: ChannelGraphItem(name), output(output)
{
	setZValue(-1);
}

void CopyFilterGUIChannelItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QColor color;
	if (output)
		color = QColor(255, 194, 194);
	else
		color = QColor(176, 255, 176);

	ChannelGraphItem::paint(painter, color);
}

QPointF CopyFilterGUIChannelItem::getPerimeterPointTo(QPointF oppositePoint)
{
	QRectF rect = boundingRect();

	if (output)
		return scenePos() + QPointF(rect.center().x(), rect.top());
	else
		return scenePos() + QPointF(rect.center().x(), rect.top() + rect.height());
}

CopyFilterGUIChannelItem* CopyFilterGUIChannelItem::findAt(QPointF mousePos)
{
	CopyFilterGUIChannelItem* result = NULL;

	QList<QGraphicsItem*> items = scene()->items(mousePos);
	for (QGraphicsItem* item : items)
	{
		CopyFilterGUIChannelItem* channelItem = qgraphicsitem_cast<CopyFilterGUIChannelItem*>(item);
		if (channelItem != NULL && channelItem->output != output)
		{
			result = channelItem;
			break;
		}
	}

	return result;
}

QLineF CopyFilterGUIChannelItem::getLineTo(CopyFilterGUIChannelItem* item)
{
	QPointF endPoint = item->getPerimeterPointTo(scenePos());

	QPointF startPoint = getPerimeterPointTo(endPoint);

	QLineF line;
	if (output)
		line = QLineF(endPoint, startPoint);
	else
		line = QLineF(startPoint, endPoint);

	return line;
}

void CopyFilterGUIChannelItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
}

void CopyFilterGUIChannelItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	if (currentConnection == NULL)
	{
		if (!boundingRect().contains(event->pos()))
		{
			currentConnection = new CopyFilterGUIConnectionItem();
			scene()->addItem(currentConnection);
		}
	}

	if (currentConnection != NULL)
	{
		CopyFilterGUIChannelItem* item = findAt(event->scenePos());

		if (item == NULL)
		{
			QLineF line;
			if (output)
				line = QLineF(event->scenePos(), getPerimeterPointTo(event->scenePos()));
			else
				line = QLineF(getPerimeterPointTo(event->scenePos()), event->scenePos());
			currentConnection->setLine(line);
		}
		else
		{
			currentConnection->setLine(getLineTo(item));
		}
	}
}

void CopyFilterGUIChannelItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (currentConnection != NULL)
	{
		CopyFilterGUIChannelItem* item = findAt(event->scenePos());
		if (item != NULL)
		{
			CopyFilterGUIChannelItem* source;
			CopyFilterGUIChannelItem* target;
			if (output)
			{
				source = item;
				target = this;
			}
			else
			{
				source = this;
				target = item;
			}

			double factor = 1.0;
			if (source->getName() == "")
				factor = 0.0;

			scene()->addItem(new CopyFilterGUIConnectionItem(source, target, factor, false));

			emit((CopyFilterGUIScene*)scene())->updateModel();
			emit((CopyFilterGUIScene*)scene())->updateChannels();
		}

		scene()->removeItem(currentConnection);
		delete currentConnection;
		currentConnection = NULL;
	}
	else
	{
		scene()->clearSelection();
		QList<QGraphicsItem*> items = scene()->items();
		for (QGraphicsItem* item : items)
		{
			CopyFilterGUIConnectionItem* connItem = qgraphicsitem_cast<CopyFilterGUIConnectionItem*>(item);
			if (connItem != NULL)
			{
				if (connItem->getSource() == this || connItem->getTarget() == this)
					connItem->setSelected(true);
			}
		}
	}
}
