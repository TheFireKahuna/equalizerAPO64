/*
    This file is part of EqualizerAPO, a system-wide equalizer.
    Copyright (C) 2014  Jonas Thedering

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

#include "Editor/IFilterGUI.h"

namespace Ui {
class CommentFilterGUI;
}

class CommentFilterGUI : public IFilterGUI
{
	Q_OBJECT

public:
	explicit CommentFilterGUI(IFilterGUI* child, bool isComment);
	~CommentFilterGUI();
	void configureChannels(std::vector<std::wstring>& channelNames) override;
	void store(QString& command, QString& parameters) override;

	void loadPreferences(const QVariantMap& prefs) override;
	void storePreferences(QVariantMap& prefs) override;

private slots:
	void on_actionPowerOn_toggled(bool checked);

private:
	Ui::CommentFilterGUI* ui;
	IFilterGUI* child;
};
