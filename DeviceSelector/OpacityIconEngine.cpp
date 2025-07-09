/*
	This file is part of EqualizerAPO, a system-wide equalizer.
	Copyright (C) 2024  Jonas Thedering

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

#include "stdafx.h"
#include "OpacityIconEngine.h"

OpacityIconEngine::OpacityIconEngine(const QIcon& icon)
	:icon(icon)
{
}

void OpacityIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
	painter->setOpacity(opacity);
	icon.paint(painter, rect, Qt::AlignCenter, mode, state);
}

OpacityIconEngine* OpacityIconEngine::clone() const
{
	OpacityIconEngine* engine = new OpacityIconEngine(icon);
	engine->setOpacity(opacity);
	return engine;
}

void OpacityIconEngine::setOpacity(qreal opacity)
{
	this->opacity = opacity;
}
