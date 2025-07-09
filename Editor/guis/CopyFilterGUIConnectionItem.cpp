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
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QPainter>
#include <QLineEdit>

#include "Editor/helpers/GUIHelper.h"
#include "Editor/widgets/ResizingLineEdit.h"
#include "CopyFilterGUIChannelItem.h"
#include "CopyFilterGUIScene.h"
#include "CopyFilterGUIConnectionItem.h"

static const int maxArrowSize = 6;

CopyFilterGUIConnectionItem::CopyFilterGUIConnectionItem()
{
}

CopyFilterGUIConnectionItem::CopyFilterGUIConnectionItem(CopyFilterGUIChannelItem* source, CopyFilterGUIChannelItem* target, double factor, bool isDecibel)
{
	this->source = source;
	this->target = target;
	this->factor = factor;
	this->isDecibel = isDecibel;

	setLine(source->getLineTo(target));
	setFlag(ItemIsSelectable);
	setAcceptHoverEvents(true);
}

QRectF CopyFilterGUIConnectionItem::boundingRect() const
{
	QRectF rect = QGraphicsLineItem::boundingRect();

	QPointF p2 = line().p2();
	int as = GUIHelper::scale(maxArrowSize);
	QRectF rect2 = QRectF(p2.x() - as, p2.y() - as, 2 * as, 2 * as);

	return rect.united(rect2);
}

QPainterPath CopyFilterGUIConnectionItem::shape() const
{
	QLineF l = line();

	QPainterPath path;
	path.setFillRule(Qt::WindingFill);
	double length = line().length();
	if (length > 0)
	{
		double offset = fmin(length, GUIHelper::scale(maxArrowSize));
		QLineF unit = l.unitVector();
		QLineF normal = l.normalVector().unitVector();
		QPointF v(unit.dx(), unit.dy());
		QPointF n(normal.dx(), normal.dy());
		QPointF p2 = l.p2();

		QPointF p3 = p2 - v * offset + 0.5 * n * offset;
		QPointF p4 = p2 - v * offset - 0.5 * n * offset;
		QPolygonF polygon;
		polygon.append(p4);
		polygon.append(p3);
		polygon.append(p2);
		path.addPolygon(polygon);

		QPolygonF polygon2;
		QPointF p1 = l.p1();
		polygon2.append(p2 + 3 * n);
		polygon2.append(p2 - 2 * n);
		polygon2.append(p1 - 2 * n);
		polygon2.append(p1 + 3 * n);
		path.addPolygon(polygon2);

		if (factor != 1.0 || isDecibel)
		{
			QFont font;
			font.setPixelSize(GUIHelper::scale(10));
			QPointF center = (l.p1() + l.p2()) / 2;
			QString text = QString("%1").arg(factor);
			if (isDecibel)
				text += " dB";

			QFontMetrics fontMetrics(font);
			QSizeF size = fontMetrics.size(0, text);
			size += QSizeF(2, 0);
			QRectF rect;
			rect.setSize(size);
			rect.moveCenter(center);
			path.addRoundedRect(rect.adjusted(-0.5, 0.5, 0.5, 0.5), 3, 3);
		}
	}

	return path;
}

bool CopyFilterGUIConnectionItem::contains(const QPointF& point) const
{
	return shape().contains(point);
}

void CopyFilterGUIConnectionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	QLineF l = line();
	if (!l.isNull())
	{
		painter->setBrush(Qt::black);
		if (isSelected())
		{
			painter->setBrush(Qt::blue);
			painter->setPen(QPen(Qt::blue, 1.5));
		}
		painter->drawLine(line());

		double length = line().length();
		double offset = fmin(length, GUIHelper::scale(maxArrowSize));
		QLineF unit = l.unitVector();
		QLineF normal = l.normalVector().unitVector();
		QPointF v(unit.dx(), unit.dy());
		QPointF n(normal.dx(), normal.dy());
		QPointF p2 = l.p2();

		QPointF p3 = p2 - v * offset + 0.5 * n * offset;
		QPointF p4 = p2 - v * offset - 0.5 * n * offset;
		QPolygonF polygon;
		polygon.append(p2);
		polygon.append(p3);
		polygon.append(p4);

		painter->drawPolygon(polygon);

		if (factor != 1.0 || isDecibel)
		{
			QFont font = painter->font();
			font.setPixelSize(GUIHelper::scale(10));
			painter->setFont(font);
			QPointF center = (l.p1() + l.p2()) / 2;
			QString text = QString("%1").arg(factor);
			if (isDecibel)
				text += " dB";

			QSizeF size = painter->fontMetrics().size(0, text);
			size += QSizeF(2, 0);
			QRectF rect;
			rect.setSize(size);
			rect.moveCenter(center);
			painter->setBrush(Qt::white);
			painter->drawRoundedRect(rect.adjusted(-0.5, 0.5, 0.5, 0.5), 3, 3);
			// make sure that text is visible in dark mode
			if(painter->pen().color() == Qt::white)
				painter->setPen(Qt::black);
			painter->drawText(rect, Qt::AlignCenter, text);
		}
	}
}

void CopyFilterGUIConnectionItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	QString text = QString("%1").arg(factor);
	if (isDecibel)
		text += " dB";

	ResizingLineEdit* lineEdit = new ResizingLineEdit(text, true);
	connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditEditingFinished()));
	connect(lineEdit, SIGNAL(editingCanceled()), this, SLOT(lineEditEditingCanceled()));
	QGraphicsProxyWidget* proxyItem = scene()->addWidget(lineEdit);
	QLineF l = line();
	QPointF center = (l.p1() + l.p2()) / 2;
	QRectF rect;
	rect.setSize(lineEdit->size());
	rect.moveCenter(center);
	proxyItem->setPos(rect.topLeft());
	proxyItem->setZValue(10);
	lineEdit->setFocus();
}

void CopyFilterGUIConnectionItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	setZValue(isSelected() ? 3 : 2);
}

void CopyFilterGUIConnectionItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	setZValue(isSelected() ? 1 : 0);
}

QVariant CopyFilterGUIConnectionItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
	if (change == ItemSelectedHasChanged)
	{
		setZValue(isSelected() ? 1 : 0);
	}

	return QGraphicsItem::itemChange(change, value);
}

void CopyFilterGUIConnectionItem::lineEditEditingFinished()
{
	QLineEdit* lineEdit = qobject_cast<QLineEdit*>(QObject::sender());
	// might be called twice
	if (!lineEdit->isVisible())
		return;

	QString text = lineEdit->text();
	text.replace(',', '.');
	bool ok = false;
	int lengthBefore = text.length();
	text.replace("db", "", Qt::CaseInsensitive);
	double value = text.toDouble(&ok);
	if (ok)
	{
		factor = value;
		isDecibel = text.length() < lengthBefore;
	}

	lineEdit->setVisible(false);
	lineEdit->deleteLater();

	emit((CopyFilterGUIScene*)scene())->updateModel();
}

void CopyFilterGUIConnectionItem::lineEditEditingCanceled()
{
	QLineEdit* lineEdit = qobject_cast<QLineEdit*>(QObject::sender());

	lineEdit->setVisible(false);
	lineEdit->deleteLater();
}

CopyFilterGUIChannelItem* CopyFilterGUIConnectionItem::getSource() const
{
	return source;
}

CopyFilterGUIChannelItem* CopyFilterGUIConnectionItem::getTarget() const
{
	return target;
}

double CopyFilterGUIConnectionItem::getFactor() const
{
	return factor;
}

bool CopyFilterGUIConnectionItem::getIsDecibel() const
{
	return isDecibel;
}
