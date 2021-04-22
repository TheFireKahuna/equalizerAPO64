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

#include "helpers/GainIterator.h"
#include "AnalysisPlotScene.h"
#include "AnalysisPlotView.h"

using namespace std;

AnalysisPlotView::AnalysisPlotView(QWidget* parent)
	: FrequencyPlotView(parent)
{
}

void AnalysisPlotView::drawBackground(QPainter* painter, const QRectF& rect)
{
	FrequencyPlotView::drawBackground(painter, rect);

	painter->setRenderHint(QPainter::Antialiasing, true);
	AnalysisPlotScene* s = qobject_cast<AnalysisPlotScene*>(scene());
	std::vector<FilterNode> nodes = s->getNodes();
	GainIterator gainIterator(nodes);
	QPainterPath path;
	bool first = true;
	double lastDb = -1000;
	for (int x = rect.left() - 1; x <= rect.right() + 1; x++)
	{
		double hz = s->xToHz(x);
		double db = gainIterator.gainAt(hz);
		double y = s->dbToY(db);
		if (y == -1)
		{
			if (db < 0)
				y = sceneRect().bottom() + 1;
			else
				y = sceneRect().top() - 1;
		}

		if (db == lastDb)
			y = floor(y) + 0.5;
		lastDb = db;
		if (first)
		{
			path.moveTo(x, y);
			first = false;
		}
		else
		{
			path.lineTo(x, y);
		}
	}

	path.lineTo(rect.right() + 1, rect.bottom() + 1);
	path.lineTo(rect.left() - 1, rect.bottom() + 1);

	double thresholdY = s->dbToY(0);
	if (rect.top() < thresholdY)
	{
		QPainterPath rectPath;
		rectPath.addRect(rect.left(), rect.top(), rect.width(), min(thresholdY - rect.top(), rect.height()));
		QPainterPath clippingPath = path.intersected(rectPath);
		painter->setPen(Qt::NoPen);
		painter->setBrush(Qt::red);
		painter->drawPath(clippingPath);
		painter->setBrush(Qt::NoBrush);
	}

	painter->setPen(Qt::black);
	painter->drawPath(path);
}
