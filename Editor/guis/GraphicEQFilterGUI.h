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

#include <QRegularExpression>

#include "Editor/IFilterGUI.h"
#include "filters/GraphicEQFilter.h"
#include "GraphicEQFilterGUIScene.h"

class FilterTable;

namespace Ui {
class GraphicEQFilterGUI;
}

class GraphicEQFilterGUI : public IFilterGUI
{
	Q_OBJECT

public:
	explicit GraphicEQFilterGUI(GraphicEQFilter* filter, QString configPath, FilterTable* filterTable);
	~GraphicEQFilterGUI();

	void store(QString& command, QString& parameters) override;

	void loadPreferences(const QVariantMap& prefs) override;
	void storePreferences(QVariantMap& prefs) override;

private slots:
	void insertRow(int index, double hz, double db);
	void removeRow(int index);
	void updateRow(int index, double hz, double db);
	void moveRow(int fromIndex, int toIndex);
	void selectRow(int index, bool select);

	void on_tableWidget_cellChanged(int row, int column);
	void on_tableWidget_itemSelectionChanged();

	void on_radioButton15_toggled(bool checked);
	void on_radioButton31_toggled(bool checked);
	void on_radioButtonVar_toggled(bool checked);

	void on_actionImport_triggered();
	void on_actionExport_triggered();
	void on_actionInvertResponse_triggered();
	void on_actionNormalizeResponse_triggered();
	void on_actionResetResponse_triggered();

private:
	void setFreqEditable(bool editable);

	Ui::GraphicEQFilterGUI* ui;
	GraphicEQFilterGUIScene* scene;
	QString configPath;
	static QRegularExpression numberRegEx;
};
