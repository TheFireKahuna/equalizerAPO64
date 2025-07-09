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

#include "Editor/helpers/GUIHelper.h"
#include "ChannelFilterGUIChannelItem.h"
#include "ChannelFilterGUIScene.h"

using namespace std;

ChannelFilterGUIScene::ChannelFilterGUIScene()
{
}

void ChannelFilterGUIScene::load(vector<wstring> channelNames, QStringList selectedChannels)
{
	blockSignals(true);
	clear();

	QHash<QString, ChannelFilterGUIChannelItem*> channelMap;

	QGraphicsItem* lastItem = NULL;

	channelNames.push_back(L"ALL");

	for (unsigned i = 0; i < channelNames.size(); i++)
	{
		QString c = QString::fromStdWString(channelNames[i]);

		ChannelFilterGUIChannelItem* item = new ChannelFilterGUIChannelItem(c);
		item->setPos(getNextChannelPoint(lastItem, false));
		addItem(item);

		channelMap.insert(c, item);
		if (c == "LFE")
			channelMap.insert("SUB", item);
		if (c != "ALL")
			channelMap.insert(QString().setNum(i + 1), item);
		lastItem = item;
	}

	for (QString c : selectedChannels)
	{
		ChannelFilterGUIChannelItem* item = channelMap.value(c.toUpper());
		if (item == NULL)
		{
			item = new ChannelFilterGUIChannelItem(c);
			item->setPos(getNextChannelPoint(lastItem, false));
			addItem(item);

			channelMap.insert(c, item);
			lastItem = item;
		}
		item->setSelected(true);
	}

	int margin = GUIHelper::scale(4);
	setSceneRect(itemsBoundingRect().marginsAdded(QMarginsF(margin, margin, margin, margin)));
	blockSignals(false);
}

QStringList ChannelFilterGUIScene::getSelectedChannels()
{
	QStringList selectedChannels;

	for (QGraphicsItem* item : items(Qt::AscendingOrder))
	{
		ChannelFilterGUIChannelItem* channelItem = qgraphicsitem_cast<ChannelFilterGUIChannelItem*>(item);
		if (channelItem != NULL)
		{
			if (channelItem->isSelected())
				selectedChannels.append(channelItem->getName());
		}
	}

	return selectedChannels;
}
