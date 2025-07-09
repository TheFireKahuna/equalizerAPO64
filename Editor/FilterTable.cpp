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

#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QElapsedTimer>
#include <QLineEdit>
#include <QToolButton>
#include <QScrollBar>
#include <QToolBar>
#include <QComboBox>
#include <QAbstractSpinBox>
#include <QDial>
#include <QJsonDocument>
#include <QSettings>

#include "MainWindow.h"
#include "FilterTableRow.h"
#include "FilterTableMimeData.h"
#include "guis/ExpressionFilterGUIFactory.h"
#include "guis/CommentFilterGUIFactory.h"
#include "guis/DeviceFilterGUIFactory.h"
#include "guis/ChannelFilterGUIFactory.h"
#include "guis/StageFilterGUIFactory.h"
#include "guis/PreampFilterGUIFactory.h"
#include "guis/BiQuadFilterGUIFactory.h"
#include "guis/CopyFilterGUIFactory.h"
#include "guis/DelayFilterGUIFactory.h"
#include "guis/IncludeFilterGUIFactory.h"
#include "guis/GraphicEQFilterGUIFactory.h"
#include "guis/ConvolutionFilterGUIFactory.h"
#include "guis/VSTPluginFilterGUIFactory.h"
#include "guis/LoudnessCorrectionFilterGUIFactory.h"
#include "Editor/helpers/GUIHelper.h"
#include "helpers/StringHelper.h"
#include "helpers/LogHelper.h"
#include "helpers/ChannelHelper.h"
#include "helpers/RegistryHelper.h"
#include "FilterTable.h"

using namespace std;

FilterTable::FilterTable(MainWindow* mainWindow, QWidget* parent)
	: QWidget(parent), mainWindow(mainWindow)
{
	gridLayout = new QGridLayout(this);

	QIcon icon(QStringLiteral(":/icons/arrow_right.ico"));
	insertArrow = new QLabel(this);
	insertArrow->setPixmap(icon.pixmap(GUIHelper::scale(QSize(24, 15))));
	insertArrow->setVisible(false);

	factories.append(new ExpressionFilterGUIFactory);
	factories.append(new CommentFilterGUIFactory);
	factories.append(new IncludeFilterGUIFactory);
	factories.append(new DeviceFilterGUIFactory);
	factories.append(new ChannelFilterGUIFactory);
	factories.append(new StageFilterGUIFactory);
	factories.append(new PreampFilterGUIFactory);
	factories.append(new BiQuadFilterGUIFactory);
	factories.append(new DelayFilterGUIFactory);
	factories.append(new CopyFilterGUIFactory);
	factories.append(new GraphicEQFilterGUIFactory);
	factories.append(new ConvolutionFilterGUIFactory);
	factories.append(new VSTPluginFilterGUIFactory);
	factories.append(new LoudnessCorrectionFilterGUIFactory);

	QApplication::instance()->installEventFilter(this);
}

FilterTable::~FilterTable()
{
	for (IFilterGUIFactory* factory : factories)
		delete factory;
	factories.clear();
}

void FilterTable::initialize(QScrollArea* scrollArea, const QList<shared_ptr<AbstractAPOInfo>>& outputDevices, const QList<shared_ptr<AbstractAPOInfo>>& inputDevices)
{
	this->scrollArea = scrollArea;
	this->outputDevices = outputDevices;
	this->inputDevices = inputDevices;

	for (IFilterGUIFactory* factory : factories)
		factory->initialize(this);
}

void FilterTable::updateDeviceAndChannelMask(shared_ptr<AbstractAPOInfo> selectedDevice, int channelMask)
{
	this->selectedDevice = selectedDevice;
	this->selectedChannelMask = channelMask;

	if (!items.empty())
		updateGuis();
}

void FilterTable::updateGuis()
{
	QElapsedTimer timer;
	timer.start();

	for (Item* item : items)
	{
		if (item->gui != NULL)
		{
			item->prefs.clear();
			item->gui->storePreferences(item->prefs);
		}
	}

	delete layout();

	for (QObject* object : children())
	{
		if (object != insertArrow)
		{
			QWidget* widget = qobject_cast<QWidget*>(object);
			if (widget != NULL)
				widget->setVisible(false);
			object->deleteLater();
		}
	}

	qDebug("Delete took %d ms", timer.elapsed());
	timer.start();

	gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setSpacing(0);
	gridLayout->setColumnStretch(0, 0);
	gridLayout->setColumnStretch(1, 1);

	for (IFilterGUIFactory* factory : factories)
		factory->startOfFile(configPath);

	int row = 0;
	for (Item* item : items)
	{
		QString line = item->text;
		IFilterGUI* gui = NULL;
		int pos = line.indexOf(':');
		if (pos != -1)
		{
			QString key = line.mid(0, pos);
			QString value = line.mid(pos + 1);

			// allow to use indentation
			key = key.trimmed();
			QString factoryKey = key;
			QString factoryValue = value;

			for (IFilterGUIFactory* factory : factories)
			{
				gui = factory->createFilterGUI(factoryKey, factoryValue);

				if (gui != NULL || factoryKey == "")
					break;
			}

			if (gui != NULL)
			{
				for (IFilterGUIFactory* factory : factories)
				{
					gui = factory->decorateFilterGUI(gui);
				}
			}
		}

		FilterTableRow* rowWidget = new FilterTableRow(this, row + 1, item, gui);
		gridLayout->addWidget(rowWidget, row, 0);

		item->gui = gui;

		if (gui != NULL)
		{
			gui->loadPreferences(item->prefs);

			connect(gui, SIGNAL(updateModel()), this, SLOT(updateModel()));
			connect(gui, SIGNAL(updateChannels()), this, SLOT(updateChannels()));
		}

		row++;
	}

	for (IFilterGUIFactory* factory : factories)
		factory->endOfFile(configPath);

	propagateChannels();

	QToolBar* toolBar = new QToolBar;
	toolBar->setIconSize(GUIHelper::scale(QSize(16, 16)));

	QWidget* spacer = new QWidget;
	spacer->setFixedWidth(GUIHelper::scale(25));
	toolBar->addWidget(spacer);

	QAction* addAction = new QAction(QIcon(":/icons/list-add-green.ico"), tr("Add filter"), toolBar);
	addAction->setCheckable(true);
	connect(addAction, SIGNAL(triggered()), this, SLOT(addActionTriggered()));
	toolBar->addAction(addAction);

	gridLayout->addWidget(toolBar, row++, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop);

	QSpacerItem* spacerItem = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
	gridLayout->addItem(spacerItem, row, 0);

	gridLayout->setRowStretch(row, 1);

	disableWheelForWidgets();

	qDebug("Create took %d ms", timer.elapsed());
	update();
}

void FilterTable::propagateChannels()
{
	vector<wstring> channelNames;
	if (selectedDevice != NULL)
		channelNames = ChannelHelper::getChannelNames(selectedDevice->getChannelCount(), selectedChannelMask);

	for (Item* item : items)
	{
		if (item->gui != NULL)
			item->gui->configureChannels(channelNames);
	}
}

QList<QString> FilterTable::getLines()
{
	QList<QString> result;
	for (Item* item : items)
		result.append(item->text);

	return result;
}

void FilterTable::setLines(const QString& configPath, const QList<QString>& lines)
{
	this->configPath = configPath;

	qDeleteAll(items);
	items.clear();

	for (QString line : lines)
	{
		items.append(new Item(line));
	}

	QSettings settings(QString::fromWCharArray(EDITOR_PER_FILE_REGPATH), QSettings::NativeFormat);
	settings.beginGroup(QString(configPath).replace('\\', '|'));
	QVariant prefsValue = settings.value("rowPrefs");
	QStringList prefLines;
	if (prefsValue.isValid())
		prefLines = prefsValue.toStringList();
	for (QString prefLine : prefLines)
	{
		int index = prefLine.indexOf(':');
		int lineNumber;
		if (index != -1)
			lineNumber = prefLine.left(index).toInt();

		QString prefCommand;
		QString prefString;
		if (lineNumber > 0)
		{
			int index2 = prefLine.indexOf(':', index + 1);
			if (index2 != -1)
			{
				prefCommand = prefLine.mid(index + 1, index2 - index - 1);
				prefString = prefLine.mid(index2 + 1);

				if (lineNumber <= items.size())
				{
					Item* item = items[lineNumber - 1];

					QString command;
					int index = item->text.indexOf(':');
					if (index != -1)
						command = item->text.left(index).trimmed();

					if (command == prefCommand)
						item->prefs = QJsonDocument::fromJson(prefString.toUtf8()).toVariant().toMap();
				}
			}
		}
	}
	setScrollOffsets(settings.value("scrollX", 0).toInt(), settings.value("scrollY", 0).toInt());
	settings.endGroup();

	if (!items.isEmpty())
	{
		focused = items[0];
		selectionStart = items[0];
	}
	else
	{
		focused = NULL;
		selectionStart = NULL;
	}

	updateGuis();
}

FilterTable::Item* FilterTable::addLine(const QString& line, FilterTable::Item* before)
{
	Item* newItem = new Item(line);

	if (before != NULL)
	{
		int index = items.indexOf(before);
		items.insert(index, newItem);
	}
	else
	{
		items.append(newItem);
	}

	emit linesChanged();

	return newItem;
}

void FilterTable::removeItem(FilterTable::Item* item)
{
	items.removeOne(item);
	emit linesChanged();
}

QMenu* FilterTable::createAddPopupMenu()
{
	QHash<QList<QString>, QMenu*> pathMap;
	QMenu* rootMenu = new QMenu;
	pathMap[QStringList()] = rootMenu;

	for (IFilterGUIFactory* f : factories)
	{
		QList<FilterTemplate> templates = f->createFilterTemplates();
		for (FilterTemplate t : templates)
		{
			QMenu* menu = pathMap.value(t.getPath());
			if (menu == NULL)
			{
				QMenu* parentMenu = rootMenu;
				QStringList currentPath;
				for (QString pathSegment : t.getPath())
				{
					currentPath.append(pathSegment);
					menu = pathMap.value(currentPath);
					if (menu == NULL)
					{
						menu = new QMenu(pathSegment);
						pathMap.insert(currentPath, menu);
						parentMenu->addMenu(menu);
					}
					parentMenu = menu;
				}
			}

			QAction* action = menu->addAction(t.getName());
			action->setData(QVariant::fromValue(t));
		}
	}

	return rootMenu;
}

void FilterTable::cut()
{
	copy();
	deleteSelectedLines();
}

void FilterTable::copy()
{
	QString text;
	QList<QVariantMap> prefsList;
	bool first = true;
	for (Item* item : items)
	{
		if (selected.contains(item))
		{
			if (first)
				first = false;
			else
				text += "\n";
			text += item->text;
			prefsList.append(item->prefs);
		}
	}

	if (selected.size() > 0)
	{
		FilterTableMimeData* mimeData = new FilterTableMimeData;
		mimeData->setText(text);
		mimeData->setPrefsList(prefsList);
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->setMimeData(mimeData);
	}
}

void FilterTable::paste()
{
	QClipboard* clipboard = QApplication::clipboard();
	const QMimeData* mimeData = clipboard->mimeData();
	if (mimeData->hasText())
	{
		int dropRow = items.size();
		for (int i = 0; i < items.size(); i++)
		{
			if (selected.contains(items[i]))
			{
				dropRow = i;
				break;
			}
		}

		QString text = mimeData->text();
		QStringList textLines = text.split("\n");
		QList<QVariantMap> prefsList;
		const FilterTableMimeData* filterTableMimeData = qobject_cast<const FilterTableMimeData*>(mimeData);
		if (filterTableMimeData != NULL)
			prefsList = filterTableMimeData->getPrefsList();

		selected.clear();
		focused = NULL;
		selectionStart = NULL;
		for (int i = 0; i < textLines.size(); i++)
		{
			QString line = textLines[i];
			Item* item = new Item(line);
			if (!prefsList.isEmpty())
				item->prefs = prefsList[i];
			selected.insert(item);
			items.insert(dropRow++, item);
			if (focused == NULL)
			{
				focused = item;
				selectionStart = item;
			}
		}

		emit linesChanged();
		updateGuis();
	}
}

void FilterTable::deleteSelectedLines()
{
	QList<Item*> newItems;
	for (Item* item : items)
	{
		if (selected.contains(item))
		{
			if (item == focused)
				focused = NULL;
			if (item == selectionStart)
				selectionStart = NULL;
			delete item;
		}
		else
		{
			newItems.append(item);
		}
	}
	selected.clear();
	items = newItems;
	emit linesChanged();
	updateGuis();
}

void FilterTable::selectAll()
{
	selected.clear();
	for (Item* item : items)
		selected.insert(item);
	update();
}

void FilterTable::updateModel()
{
	emit linesChanged();
}

void FilterTable::updateChannels()
{
	propagateChannels();
}

void FilterTable::addActionTriggered()
{
	QMenu* menu = createAddPopupMenu();
	QAction* addAction = qobject_cast<QAction*>(QObject::sender());
	QToolBar* toolBar = qobject_cast<QToolBar*>(addAction->parentWidget());
	QRect rect = toolBar->actionGeometry(addAction);
	QPoint p = toolBar->mapToGlobal(QPoint(rect.x(), rect.y() + rect.height()));
	QAction* action = menu->exec(p);
	addAction->setChecked(false);
	if (action != NULL)
	{
		FilterTemplate t = action->data().value<FilterTemplate>();
		QString line = t.getLine();
		addLine(line);
		updateGuis();
	}
}

void FilterTable::openConfig(QString path)
{
	mainWindow->load(path);
}

int FilterTable::getPreferredWidth()
{
	if (scrollArea == NULL)
		return width();

	return scrollArea->viewport()->width();
}

void FilterTable::updateSizeHints()
{
	for (int i = 0; i < items.size(); i++)
	{
		FilterTableRow* tableRow = qobject_cast<FilterTableRow*>(gridLayout->itemAtPosition(i, 0)->widget());
		tableRow->updateGeometry();
	}
}

QSize FilterTable::minimumSizeHint() const
{
	QSize size = QWidget::minimumSizeHint();
	if (size.height() < minimumHeightHint)
		size.setHeight(minimumHeightHint);

	return size;
}

void FilterTable::setMinimumHeightHint(int height)
{
	minimumHeightHint = height;
	updateGeometry();
}

void FilterTable::savePreferences()
{
	if (!configPath.isEmpty())
	{
		QStringList prefLines;

		for (int i = 0; i < items.size(); i++)
		{
			Item* item = items[i];

			if (item->gui != NULL)
			{
				item->prefs.clear();
				item->gui->storePreferences(item->prefs);
			}

			if (!item->prefs.isEmpty())
			{
				QString command;
				int index = item->text.indexOf(':');
				if (index != -1)
					command = item->text.left(index).trimmed();

				QByteArray byteArray = QJsonDocument::fromVariant(item->prefs).toJson(QJsonDocument::Compact);
				QString string = QString("%0:%1:%2").arg(i + 1).arg(command).arg(QString::fromUtf8(byteArray));
				prefLines.append(string);
			}
		}

		QSettings settings(QString::fromWCharArray(EDITOR_PER_FILE_REGPATH), QSettings::NativeFormat);
		settings.beginGroup(QString(configPath).replace('\\', '|'));
		settings.setValue("rowPrefs", prefLines);
		settings.setValue("scrollX", scrollArea->horizontalScrollBar()->value());
		settings.setValue("scrollY", scrollArea->verticalScrollBar()->value());
		settings.endGroup();
	}
}

void FilterTable::setScrollOffsets(int x, int y)
{
	presetScrollX = x;
	presetScrollY = y;
}

void FilterTable::updateAnalysis()
{
	if (isVisible())
		mainWindow->startAnalysis();
}

void FilterTable::mousePressEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		int row = rowForPos(event->pos(), false);
		if (row != -1)
		{
			Item* item = items[row];

			if (event->modifiers() & Qt::ControlModifier)
			{
				if (!selected.remove(item))
					selected.insert(item);
				selectionStart = item;
			}
			else if (event->modifiers() & Qt::ShiftModifier)
			{
				int startRow = items.indexOf(selectionStart);
				if (startRow != -1)
				{
					selected.clear();
					for (int i = min(startRow, row); i <= max(startRow, row); i++)
					{
						selected.insert(items[i]);
					}
				}
			}
			else
			{
				if (!selected.contains(item))
				{
					selected.clear();
					selected.insert(item);
				}
				selectionStart = item;
			}
			focused = item;
			ensureRowVisible(row);
			update();

			dragStartPos = event->pos();
		}
		else
		{
			selected.clear();
			update();
		}
	}
}

void FilterTable::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		int row = rowForPos(event->pos(), false);
		if (row != -1)
		{
			Item* item = items[row];

			if (!(event->modifiers() & Qt::ControlModifier) && !(event->modifiers() & Qt::ShiftModifier))
			{
				if (selected.contains(item) && selectionStart == item)
				{
					selected.clear();
					selected.insert(item);
				}
			}
			ensureRowVisible(row);
			update();
		}
	}
}

void FilterTable::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton)
	{
		if ((event->pos() - dragStartPos).manhattanLength() >= QApplication::startDragDistance())
		{
			QString text;
			QList<QVariantMap> prefsList;
			bool first = true;
			int i = 0;
			bool dragPosInside = false;
			for (Item* item : items)
			{
				if (selected.contains(item))
				{
					if (first)
						first = false;
					else
						text += "\n";
					text += item->text;
					if (item->gui != NULL)
						item->gui->storePreferences(item->prefs);
					prefsList.append(item->prefs);

					if (!dragPosInside)
					{
						FilterTableRow* tableRow = qobject_cast<FilterTableRow*>(gridLayout->itemAtPosition(i, 0)->widget());
						QRect rect = tableRow->getHeaderRect().translated(tableRow->pos());
						if (rect.contains(dragStartPos))
							dragPosInside = true;
					}
				}
				i++;
			}

			if (selected.size() > 0 && dragPosInside)
			{
				FilterTableMimeData* mimeData = new FilterTableMimeData;
				mimeData->setText(text);
				mimeData->setPrefsList(prefsList);

				QDrag* drag = new QDrag(this);
				drag->setMimeData(mimeData);
				QSet<Item*> selectedBefore = selected;
				internalDrag = true;
				Qt::DropAction action = drag->exec(Qt::MoveAction | Qt::CopyAction);
				internalDrag = false;
				if (action == Qt::MoveAction)
				{
					for (Item* item : selectedBefore)
					{
						items.removeOne(item);
						if (focused == item)
							focused = NULL;
						if (selectionStart == item)
							selectionStart = NULL;
						selected.remove(item);
						delete item;
					}
				}

				if (action != Qt::IgnoreAction)
				{
					emit linesChanged();
					updateGuis();
				}
			}
		}
	}

	QWidget::mouseMoveEvent(event);
}

void FilterTable::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasText())
	{
		if (event->keyboardModifiers() & Qt::ControlModifier)
			event->setDropAction(Qt::CopyAction);
		else
			event->setDropAction(Qt::MoveAction);
		event->accept();
	}

	QWidget::dragEnterEvent(event);
}

void FilterTable::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasText())
	{
		if (event->keyboardModifiers() & Qt::ControlModifier)
			event->setDropAction(Qt::CopyAction);
		else
			event->setDropAction(Qt::MoveAction);

		int dropRow = rowForPos(event->pos(), true);

		QRect rect = gridLayout->itemAtPosition(dropRow == -1 ? gridLayout->rowCount() - 2 : dropRow, 0)->geometry();
		insertArrow->move(0, rect.top() - insertArrow->height() / 2 - gridLayout->verticalSpacing() / 2);

		insertArrow->raise();
		insertArrow->show();
	}

	QWidget::dragMoveEvent(event);
}

void FilterTable::dragLeaveEvent(QDragLeaveEvent* event)
{
	insertArrow->hide();
}

void FilterTable::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData->hasText())
	{
		if (event->keyboardModifiers() & Qt::ControlModifier)
			event->setDropAction(Qt::CopyAction);
		else
			event->setDropAction(Qt::MoveAction);

		QString text = mimeData->text();
		QStringList textLines = text.split("\n");
		QList<QVariantMap> prefsList;
		const FilterTableMimeData* filterTableMimeData = qobject_cast<const FilterTableMimeData*>(mimeData);
		if (filterTableMimeData != NULL)
			prefsList = filterTableMimeData->getPrefsList();

		int dropRow = rowForPos(event->pos(), true);
		if (dropRow == -1)
			dropRow = items.size();

		selected.clear();
		focused = NULL;
		selectionStart = NULL;
		for (int i = 0; i < textLines.size(); i++)
		{
			QString line = textLines[i];
			Item* item = new Item(line);
			if (!prefsList.isEmpty())
				item->prefs = prefsList[i];
			selected.insert(item);
			items.insert(dropRow++, item);
			if (focused == NULL)
			{
				focused = item;
				selectionStart = item;
			}
		}
		event->accept();

		if (!internalDrag)
		{
			emit linesChanged();
			updateGuis();
		}
	}

	insertArrow->hide();

	QWidget::dropEvent(event);
}

void FilterTable::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Up)
	{
		if (focused != NULL)
		{
			int row = items.indexOf(focused);
			if (row != -1)
			{
				int newRow = row;
				if (event->key() == Qt::Key_Down && row + 1 < items.size())
					newRow = row + 1;
				else if (event->key() == Qt::Key_Up && row - 1 >= 0)
					newRow = row - 1;

				if (newRow != row)
				{
					focused = items[newRow];
					if (event->modifiers() & Qt::ControlModifier)
					{
					}
					else if (event->modifiers() & Qt::ShiftModifier)
					{
						int startRow = items.indexOf(selectionStart);
						if (startRow != -1)
						{
							selected.clear();
							for (int i = min(startRow, newRow); i <= max(startRow, newRow); i++)
							{
								selected.insert(items[i]);
							}
						}
					}
					else
					{
						selected.clear();
						selected.insert(focused);
						selectionStart = focused;
					}

					ensureRowVisible(newRow);
					update();
				}
			}
		}
	}

	if (event->key() == Qt::Key_Space)
	{
		if (!(event->modifiers() & Qt::ControlModifier) || !selected.remove(focused))
			selected.insert(focused);
		update();
	}

	if (event->key() == Qt::Key_F2)
	{
		if (focused != NULL)
		{
			int rowIndex = items.indexOf(focused);
			if (rowIndex != -1)
			{
				QLayoutItem* layoutItem = gridLayout->itemAtPosition(rowIndex, 0);
				FilterTableRow* tableRow = qobject_cast<FilterTableRow*>(layoutItem->widget());
				if (tableRow != NULL)
					tableRow->editText();
			}
		}
	}

	if (event->key() == Qt::Key_Delete)
	{
		deleteSelectedLines();
	}
}

void FilterTable::wheelEvent(QWheelEvent* event)
{
	scrollingNow = true;
	scrollStartPoint = event->globalPosition();

	QWidget::wheelEvent(event);
}

bool FilterTable::eventFilter(QObject* obj, QEvent* event)
{
	QEvent::Type type = event->type();
	if (scrollingNow)
	{
		if (type == QEvent::Wheel)
		{
			QWheelEvent* wheelEvent = (QWheelEvent*)event;
			scrollStartPoint = wheelEvent->globalPosition();

			QWidget* widget = qobject_cast<QWidget*>(obj);
			if (widget != NULL)
			{
				if (isAncestorOf(widget))
				{
					QApplication::sendEvent(parent(), event);
					return true;
				}
			}
		}
		else if (type == QEvent::MouseMove)
		{
			QMouseEvent* mouseEvent = (QMouseEvent*)event;

			if ((mouseEvent->globalPos() - scrollStartPoint).manhattanLength() > GUIHelper::scale(30))
				scrollingNow = false;
		}
	}

	if (obj == scrollArea && type == QEvent::Resize)
	{
		updateSizeHints();
	}

	return false;
}

void FilterTable::showEvent(QShowEvent*)
{
	if (presetScrollX != -1)
	{
		scrollArea->horizontalScrollBar()->setValue(presetScrollX);
		presetScrollX = -1;
	}

	if (presetScrollY != -1)
	{
		scrollArea->verticalScrollBar()->setValue(presetScrollY);
		presetScrollY = -1;
	}
}

void FilterTable::ensureRowVisible(int row)
{
	QScrollBar* vScrollBar = scrollArea->verticalScrollBar();
	if (vScrollBar != NULL)
	{
		QRect rect = rowRect(row).toAlignedRect();
		if (rect.top() < vScrollBar->value())
			vScrollBar->setValue(max(0, rect.top()));
		else if (rect.bottom() + 1 > vScrollBar->value() + scrollArea->viewport()->height())
			vScrollBar->setValue(min(vScrollBar->maximum(), rect.bottom() + 1 - scrollArea->viewport()->height()));
	}
}

int FilterTable::rowForPos(QPoint pos, bool insert)
{
	int row = -1;
	for (int i = 0; i < gridLayout->rowCount() - 2; i++)
	{
		QRect rect = gridLayout->itemAtPosition(i, 0)->geometry();
		int y;
		if (insert)
			y = rect.center().y();
		else
			y = rect.bottom();

		if (pos.y() <= y)
		{
			row = i;
			break;
		}
	}

	return row;
}

QRectF FilterTable::rowRect(int row)
{
	QRectF rect = gridLayout->itemAtPosition(row, 0)->geometry();
	rect = rect.marginsAdded(QMarginsF(-1.5, -1.5, -1.5, -0.5));
	return rect;
}

void FilterTable::disableWheelForWidgets()
{
	QList<QWidget*> widgets = findChildren<QWidget*>();
	for (QWidget* widget : widgets)
	{
		if (qobject_cast<QComboBox*>(widget) || qobject_cast<QAbstractSpinBox*>(widget) || qobject_cast<QDial*>(widget))
		{
			widget->installEventFilter(new DisableWheelFilter(this, widget));
			if (widget->focusPolicy() == Qt::WheelFocus)
				widget->setFocusPolicy(Qt::StrongFocus);
		}
	}
}

QString FilterTable::getConfigPath() const
{
	return configPath;
}

void FilterTable::setConfigPath(const QString& value)
{
	configPath = value;
}

FilterTable::Item* FilterTable::getFocusedItem() const
{
	return focused;
}

const QSet<FilterTable::Item*>& FilterTable::getSelectedItems() const
{
	return selected;
}

const QList<shared_ptr<AbstractAPOInfo>>& FilterTable::getOutputDevices() const
{
	return outputDevices;
}

const QList<shared_ptr<AbstractAPOInfo>>& FilterTable::getInputDevices() const
{
	return inputDevices;
}

shared_ptr<AbstractAPOInfo> FilterTable::getSelectedDevice() const
{
	return selectedDevice;
}

int FilterTable::getSelectedChannelMask() const
{
	return selectedChannelMask;
}
