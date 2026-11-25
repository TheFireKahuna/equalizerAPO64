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
#include <cmath>

#include "helpers/GainIterator.h"
#include "helpers/SafetyLimits.h"
#include "Editor/helpers/GUIHelper.h"
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
	const std::vector<FilterNode>& nodes = s->getNodes();
	if (nodes.empty())
		return;

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

		if (abs(db - lastDb) < 0.001f)
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

	double zeroDbY = s->dbToY(0.0);
	double bypassLimitY = s->dbToY(SafetyLimits::MAX_PEAK_GAIN_DB);

	// Mild clipping zone (0 dB .. +5.0 dB): warm amber fill
	if (zeroDbY != -1 && bypassLimitY != -1 && rect.top() < zeroDbY && rect.bottom() > bypassLimitY)
	{
		double bandTop = std::max(rect.top(), bypassLimitY);
		double bandBottom = std::min(rect.bottom(), zeroDbY);
		if (bandBottom > bandTop)
		{
			QPainterPath mildRectPath;
			mildRectPath.addRect(rect.left(), bandTop, rect.width(), bandBottom - bandTop);
			QPainterPath mildClipPath = path.intersected(mildRectPath);
			painter->setPen(Qt::NoPen);
			painter->setBrush(QColor(255, 152, 0, 175));
			painter->drawPath(mildClipPath);
			painter->setBrush(Qt::NoBrush);
		}
	}

	// Safety bypass exceedance zone (> +5.0 dB): bright red fill
	if (bypassLimitY != -1 && rect.top() < bypassLimitY)
	{
		QPainterPath bypassRectPath;
		bypassRectPath.addRect(rect.left(), rect.top(), rect.width(), min(bypassLimitY - rect.top(), rect.height()));
		QPainterPath bypassClipPath = path.intersected(bypassRectPath);
		painter->setPen(Qt::NoPen);
		painter->setBrush(QColor(229, 57, 53, 220));
		painter->drawPath(bypassClipPath);
		painter->setBrush(Qt::NoBrush);
	}

	// Draw dashed +5.0 dB safety ceiling reference line
	if (bypassLimitY != -1 && bypassLimitY >= rect.top() - 2 && bypassLimitY <= rect.bottom() + 2)
	{
		QPen ceilingPen(QColor(229, 57, 53, 210));
		ceilingPen.setStyle(Qt::DashLine);
		ceilingPen.setWidthF(1.2);
		painter->setPen(ceilingPen);
		double lineY = floor(bypassLimitY) + 0.5;
		painter->drawLine(QPointF(rect.left(), lineY), QPointF(rect.right(), lineY));
	}

	bool dark = GUIHelper::isDarkMode();
	painter->setPen(dark ? Qt::white : Qt::black);
	painter->drawPath(path);
}
