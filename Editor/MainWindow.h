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

#include <string>
#include <vector>
#include <QMainWindow>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>

#include "FilterTable.h"
#include "DeviceAPOInfo.h"
#include "Editor/AnalysisPlotScene.h"
#include "Editor/AnalysisThread.h"
#include "helpers/RegistryHelper.h"

#define EDITOR_REGPATH USER_REGPATH L"\\Configuration Editor"
#define EDITOR_PER_FILE_REGPATH EDITOR_REGPATH L"\\file-specific"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QDir configDir, QWidget* parent = 0);
	~MainWindow();
	void doChecks();
	void runDeviceSelector();
	void load(QString path);
	void save(FilterTable* filterTable, QString path);
	bool isEmpty();
	bool shouldRestart();
	void startAnalysis();

protected:
	void closeEvent(QCloseEvent* event) override;

private slots:
	void deviceSelected(int index);
	void channelConfigurationSelected(int index);
	void linesChanged();

	bool on_tabWidget_tabCloseRequested(int index);
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionNew_triggered();
	void recentFileSelected();

	void on_actionCut_triggered();
	void on_actionCopy_triggered();
	void on_actionPaste_triggered();
	void on_actionDelete_triggered();
	void on_actionSelectAll_triggered();

	void instantModeEnabled(bool enabled);
	void on_tabWidget_currentChanged(int index);
	void on_startFromComboBox_activated(int index);
	void on_analysisChannelComboBox_activated(int index);
	void on_resolutionSpinBox_valueChanged(int value);
	void updateAnalysisPanel();

	void on_mainToolBar_visibilityChanged(bool visible);
	void on_analysisDockWidget_visibilityChanged(bool visible);
	void on_actionToolbar_triggered(bool checked);
	void on_actionAnalysisPanel_triggered(bool checked);

	void languageSelected(bool selected);
	void on_actionResetAllGlobalPreferences_triggered();
	void on_actionResetAllFileSpecificPreferences_triggered();

private:
	FilterTable* addTab(QString title, QString tooltip, QString configPath, QList<QString> lines);
	void getDeviceAndChannelMask(std::shared_ptr<AbstractAPOInfo>* selectedDevice, int* channelMask);
	bool askForClose(int tabIndex);
	void loadPreferences();
	void savePreferences();
	void updateRecentFiles();
	template<class T> QList<T> toQList(const std::vector<T>& vector);

	Ui::MainWindow* ui;

	QDir configDir;
	QCheckBox* instantModeCheckBox;
	QComboBox* deviceComboBox;
	QComboBox* channelConfigurationComboBox;
	QList<std::shared_ptr<AbstractAPOInfo>> outputDevices;
	QList<std::shared_ptr<AbstractAPOInfo>> inputDevices;
	std::shared_ptr<AbstractAPOInfo> defaultOutputDevice;
	AnalysisPlotScene* analysisPlotScene;
	AnalysisThread* analysisThread = NULL;
	bool restart = false;
	bool noSavePreferences = false;
	bool noSaveFilePreferences = false;
	QStringList recentFiles;
};

Q_DECLARE_METATYPE(std::shared_ptr<AbstractAPOInfo>)
