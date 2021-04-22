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

#include <QCheckBox>
#include <QDialog>

namespace Ui {
class ChannelFilterGUIDialog;
}

class ChannelFilterGUIDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChannelFilterGUIDialog(QWidget* parent, QStringList selectedChannels, int selectedChannelMask, const std::vector<std::wstring>& channelNames);
	~ChannelFilterGUIDialog();

	QStringList getSelectedChannels() const;

private slots:
	void on_addButton_clicked();

	void on_removeButton_clicked();

	void on_allChannelsCheckBox_toggled(bool checked);

private:
	Ui::ChannelFilterGUIDialog* ui;
	QList<QCheckBox*> checkBoxes;
};
