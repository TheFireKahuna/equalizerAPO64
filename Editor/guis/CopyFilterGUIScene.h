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

#include <vector>
#include <QGraphicsScene>

#include "Editor/widgets/ChannelGraphScene.h"
#include "filters/CopyFilter.h"

class CopyFilterGUIScene : public ChannelGraphScene
{
	Q_OBJECT

public:
	void load(const std::vector<std::wstring>& channelNames, std::vector<Assignment> assignments);
	std::vector<Assignment> buildAssignments();

signals:
	void updateModel();
	void updateChannels();

protected:
	void keyPressEvent(QKeyEvent* event) override;

private slots:
	void addOutputChannel();
	void lineEditEditingFinished();
	void lineEditEditingCanceled();

private:
	QGraphicsItem* lastOutputItem;
	QGraphicsProxyWidget* addProxyItem;
};
