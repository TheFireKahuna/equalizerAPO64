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

#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QMouseEvent>
#include <QGridLayout>
#include <QScrollArea>
#include <QMenu>
#include <QJsonObject>

#include "Editor/helpers/DisableWheelFilter.h"
#include "DeviceAPOInfo.h"
#include "IFilterGUI.h"
#include "IFilterGUIFactory.h"

class MainWindow;

class FilterTable : public QWidget
{
	Q_OBJECT
public:
	struct Item
	{
		Item()
		{
		}

		Item(const QString& text)
		{
			this->text = text;
		}

		QString text;
		QVariantMap prefs;
		IFilterGUI* gui = NULL;
	};

	explicit FilterTable(MainWindow* mainWindow, QWidget* parent = 0);
	~FilterTable();
	void initialize(QScrollArea* scrollArea, const QList<std::shared_ptr<AbstractAPOInfo>>& outputDevices, const QList<std::shared_ptr<AbstractAPOInfo>>& inputDevices);
	void updateDeviceAndChannelMask(std::shared_ptr<AbstractAPOInfo> selectedDevice, int selectedChannelMask);
	void updateGuis();
	void propagateChannels();
	QList<QString> getLines();
	void setLines(const QString& configPath, const QList<QString>& lines);
	Item* addLine(const QString& line, Item* before = NULL);
	void removeItem(Item* item);
	QMenu* createAddPopupMenu();
	void cut();
	void copy();
	void paste();
	void deleteSelectedLines();
	void selectAll();

	const QList<std::shared_ptr<AbstractAPOInfo>>& getOutputDevices() const;
	const QList<std::shared_ptr<AbstractAPOInfo>>& getInputDevices() const;
	std::shared_ptr<AbstractAPOInfo> getSelectedDevice() const;
	int getSelectedChannelMask() const;

	const QSet<Item*>& getSelectedItems() const;
	Item* getFocusedItem() const;

	QString getConfigPath() const;
	void setConfigPath(const QString& value);

	void openConfig(QString path);

	int getPreferredWidth();
	void updateSizeHints();

	QSize minimumSizeHint() const override;
	void setMinimumHeightHint(int height);

	void savePreferences();
	void setScrollOffsets(int x, int y);

	void updateAnalysis();

signals:
	void linesChanged();

public slots:
	void updateModel();
	void updateChannels();
	void addActionTriggered();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;
	void showEvent(QShowEvent*) override;

private:
	void ensureRowVisible(int row);
	int rowForPos(QPoint pos, bool insert);
	QRectF rowRect(int row);
	void disableWheelForWidgets();

	MainWindow* mainWindow;
	QScrollArea* scrollArea;
	QGridLayout* gridLayout;
	QLabel* insertArrow;
	QPoint dragStartPos;
	bool internalDrag = false;
	QList<Item*> items;
	QSet<Item*> selected;
	Item* focused = NULL;
	Item* selectionStart = NULL;
	QList<IFilterGUIFactory*> factories;
	bool scrollingNow = false;
	QPointF scrollStartPoint;
	QList<std::shared_ptr<AbstractAPOInfo>> outputDevices;
	QList<std::shared_ptr<AbstractAPOInfo>> inputDevices;
	std::shared_ptr<AbstractAPOInfo> selectedDevice;
	int selectedChannelMask;
	QString configPath;
	int minimumHeightHint = 0;
	int presetScrollX = -1;
	int presetScrollY = -1;
};

template<typename T> inline uint qHash(QList<T> list)
{
	uint hashValue = 0;

	for (T t : list)
		hashValue = 31 * hashValue + qHash(t);

	return hashValue;
}

Q_DECLARE_METATYPE(FilterTable::Item*)
