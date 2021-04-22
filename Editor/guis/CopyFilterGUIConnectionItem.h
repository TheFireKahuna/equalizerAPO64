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

#pragma once

#include <QGraphicsLineItem>

class CopyFilterGUIChannelItem;

class CopyFilterGUIConnectionItem : public QObject, public QGraphicsLineItem
{
	Q_OBJECT

public:
	CopyFilterGUIConnectionItem();
	CopyFilterGUIConnectionItem(CopyFilterGUIChannelItem* source, CopyFilterGUIChannelItem* target, double factor, bool isDecibel);

	enum {Type = UserType + 1};
	virtual int type() const
	{
		return Type;
	}

	QRectF boundingRect() const override;
	QPainterPath shape() const override;
	bool contains(const QPointF& point) const override;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

	CopyFilterGUIChannelItem* getSource() const;
	CopyFilterGUIChannelItem* getTarget() const;
	double getFactor() const;
	bool getIsDecibel() const;

protected:
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
	QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private slots:
	void lineEditEditingFinished();
	void lineEditEditingCanceled();

private:
	CopyFilterGUIChannelItem* source = NULL;
	CopyFilterGUIChannelItem* target = NULL;
	double factor = 1.0;
	bool isDecibel = false;
};
