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

#include <QWidget>
#include <QTime>

#include "FilterTable.h"

namespace Ui {
class FilterTableRow;
}

class FilterTableRow : public QWidget
{
	Q_OBJECT

public:
	explicit FilterTableRow(FilterTable* table, int index, FilterTable::Item* item, IFilterGUI* gui);
	~FilterTableRow();

	QRect getHeaderRect();
	void editText();
	QSize sizeHint() const override;

protected:
	void mouseDoubleClickEvent(QMouseEvent*) override;
	void paintEvent(QPaintEvent*) override;

private slots:
	void updateModel();

	void on_actionAdd_triggered();
	void on_actionRemove_triggered();
	void on_actionEditText_triggered(bool checked);

	void on_lineEdit_editingFinished();
	void on_lineEdit_editingCanceled();

private:
	Ui::FilterTableRow* ui;
	FilterTable* table;
	FilterTable::Item* item;
	IFilterGUI* gui;
	bool editingDone = false;
	QTime lastEditTime;
};
