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

#include <vector>
#include <QFileDialog>
#include <QScrollBar>
#include <QTextStream>

#include "helpers/GainIterator.h"
#include "Editor/helpers/GUIHelper.h"
#include "Editor/widgets/ResizeCorner.h"
#include "Editor/FilterTable.h"
#include "GraphicEQFilterGUIView.h"
#include "GraphicEQFilterGUI.h"
#include "ui_GraphicEQFilterGUI.h"

static const double DEFAULT_TABLE_WIDTH = 119;
static const double DEFAULT_VIEW_HEIGHT = 150;

using namespace std;

QRegularExpression GraphicEQFilterGUI::numberRegEx("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");

GraphicEQFilterGUI::GraphicEQFilterGUI(GraphicEQFilter* filter, QString configPath, FilterTable* filterTable)
	: ui(new Ui::GraphicEQFilterGUI), configPath(configPath)
{
	ui->setupUi(this);
	if(GUIHelper::isDarkMode())
	{
		ui->actionInvertResponse->setIcon(QIcon(":/icons/dark-mode/invert_response.svg"));
		ui->actionNormalizeResponse->setIcon(QIcon(":/icons/dark-mode/normalize_response.svg"));
		ui->actionResetResponse->setIcon(QIcon(":/icons/dark-mode/reset_response.svg"));
	}
	ui->tableWidget->horizontalHeader()->setMinimumSectionSize(GUIHelper::scale(10));
	ui->tableWidget->horizontalHeader()->setDefaultSectionSize(GUIHelper::scale(10));
	ui->tableWidget->verticalHeader()->setMinimumSectionSize(GUIHelper::scale(23));
	ui->tableWidget->verticalHeader()->setDefaultSectionSize(GUIHelper::scale(23));

	scene = new GraphicEQFilterGUIScene(ui->graphicsView);
	ui->graphicsView->setScene(scene);

	ResizeCorner* cornerWidget = new ResizeCorner(filterTable,
			QSize(0, ui->graphicsView->minimumHeight()), QSize(10000 - ui->tableWidget->minimumWidth(), INT_MAX),
			[this]() {
		return QSize(10000 - ui->tableWidget->width(), ui->graphicsView->height());
	},
			[this](QSize size) {
		ui->tableWidget->setFixedWidth(10000 - size.width());
		ui->graphicsView->setFixedHeight(size.height());
	}, ui->graphicsView);
	ui->graphicsView->setCornerWidget(cornerWidget);

	ui->toolBar->addAction(ui->actionImport);
	ui->toolBar->addAction(ui->actionExport);
	ui->toolBar->addAction(ui->actionInvertResponse);
	ui->toolBar->addAction(ui->actionNormalizeResponse);
	ui->toolBar->addAction(ui->actionResetResponse);

	connect(scene, SIGNAL(nodeInserted(int,double,double)), this, SLOT(insertRow(int,double,double)));
	connect(scene, SIGNAL(nodeRemoved(int)), this, SLOT(removeRow(int)));
	connect(scene, SIGNAL(nodeUpdated(int,double,double)), this, SLOT(updateRow(int,double,double)));
	connect(scene, SIGNAL(nodeMoved(int,int)), this, SLOT(moveRow(int,int)));
	connect(scene, SIGNAL(nodeSelectionChanged(int,bool)), this, SLOT(selectRow(int,bool)));
	connect(scene, SIGNAL(updateModel()), this, SIGNAL(updateModel()));

	scene->setNodes(filter->getNodes());

	int bandCount = scene->verifyBands(filter->getNodes());
	switch (bandCount)
	{
	case 15:
		ui->radioButton15->click();
		break;
	case 31:
		ui->radioButton31->click();
		break;
	default:
		ui->radioButtonVar->click();
	}

	ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

GraphicEQFilterGUI::~GraphicEQFilterGUI()
{
	delete ui;
}

void GraphicEQFilterGUI::store(QString& command, QString& parameters)
{
	command = "GraphicEQ";

	bool first = true;
	for (FilterNode node : scene->getNodes())
	{
		if (first)
			first = false;
		else
			parameters += "; ";

		parameters += QString("%0 %1").arg(node.freq).arg(node.dbGain);
	}
}

void GraphicEQFilterGUI::loadPreferences(const QVariantMap& prefs)
{
	ui->tableWidget->setFixedWidth(GUIHelper::scale(prefs.value("tableWidth", DEFAULT_TABLE_WIDTH).toDouble()));
	ui->graphicsView->setFixedHeight(GUIHelper::scale(prefs.value("viewHeight", DEFAULT_VIEW_HEIGHT).toDouble()));
	double zoomX = GUIHelper::scaleZoom(prefs.value("zoomX", 1.0).toDouble());
	double zoomY = GUIHelper::scaleZoom(prefs.value("zoomY", 1.0).toDouble());
	scene->setZoom(zoomX, zoomY);
	bool ok;
	int scrollX = GUIHelper::scale(prefs.value("scrollX").toDouble(&ok));
	if (!ok)
		scrollX = round(scene->hzToX(20));
	int scrollY = GUIHelper::scale(prefs.value("scrollY").toDouble(&ok));
	if (!ok)
		scrollY = round(scene->dbToY(22));
	ui->graphicsView->setScrollOffsets(scrollX, scrollY);
}

void GraphicEQFilterGUI::storePreferences(QVariantMap& prefs)
{
	if (GUIHelper::invScale(ui->tableWidget->width()) != DEFAULT_TABLE_WIDTH)
		prefs.insert("tableWidth", GUIHelper::invScale(ui->tableWidget->width()));
	if (GUIHelper::invScale(ui->graphicsView->height()) != DEFAULT_VIEW_HEIGHT)
		prefs.insert("viewHeight", GUIHelper::invScale(ui->graphicsView->height()));
	if (GUIHelper::invScaleZoom(scene->getZoomX()) != 1.0)
		prefs.insert("zoomX", GUIHelper::invScaleZoom(scene->getZoomX()));
	if (GUIHelper::invScaleZoom(scene->getZoomY()) != 1.0)
		prefs.insert("zoomY", GUIHelper::invScaleZoom(scene->getZoomY()));
	QScrollBar* hScrollBar = ui->graphicsView->horizontalScrollBar();
	int value = hScrollBar->value();
	if (value != round(scene->hzToX(20)))
		prefs.insert("scrollX", GUIHelper::invScale(value));
	QScrollBar* vScrollBar = ui->graphicsView->verticalScrollBar();
	value = vScrollBar->value();
	if (value != round(scene->dbToY(22)))
		prefs.insert("scrollY", GUIHelper::invScale(value));
}

void GraphicEQFilterGUI::insertRow(int index, double hz, double db)
{
	ui->tableWidget->blockSignals(true);
	ui->tableWidget->insertRow(index);
	QTableWidgetItem* freqItem = new QTableWidgetItem(QString("%0").arg(hz));
	ui->tableWidget->setItem(index, 0, freqItem);

	QTableWidgetItem* gainItem = new QTableWidgetItem(QString("%0").arg(db));
	ui->tableWidget->setItem(index, 1, gainItem);
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::removeRow(int index)
{
	ui->tableWidget->blockSignals(true);
	ui->tableWidget->removeRow(index);
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::updateRow(int index, double hz, double db)
{
	ui->tableWidget->blockSignals(true);
	QTableWidgetItem* freqItem = ui->tableWidget->item(index, 0);
	freqItem->setText(QString("%0").arg(hz));

	QTableWidgetItem* gainItem = ui->tableWidget->item(index, 1);
	gainItem->setText(QString("%0").arg(db));
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::moveRow(int fromIndex, int toIndex)
{
	ui->tableWidget->blockSignals(true);
	QTableWidgetItem* freqItem = ui->tableWidget->takeItem(fromIndex, 0);
	QTableWidgetItem* gainItem = ui->tableWidget->takeItem(fromIndex, 1);
	bool selected = ui->tableWidget->selectionModel()->isRowSelected(fromIndex, ui->tableWidget->rootIndex());
	ui->tableWidget->removeRow(fromIndex);
	ui->tableWidget->insertRow(toIndex);
	ui->tableWidget->setItem(toIndex, 0, freqItem);
	ui->tableWidget->setItem(toIndex, 1, gainItem);
	if (selected)
	{
		QModelIndex modelIndex = ui->tableWidget->model()->index(toIndex, 0);
		ui->tableWidget->selectionModel()->select(modelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
		ui->tableWidget->scrollTo(modelIndex);
	}
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::selectRow(int index, bool select)
{
	ui->tableWidget->blockSignals(true);
	QItemSelectionModel::SelectionFlags command = QItemSelectionModel::Rows;
	if (select)
		command |= QItemSelectionModel::Select;
	else
		command |= QItemSelectionModel::Deselect;
	QModelIndex modelIndex = ui->tableWidget->model()->index(index, 0);
	ui->tableWidget->selectionModel()->select(modelIndex, command);
	ui->tableWidget->scrollTo(modelIndex);
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::on_tableWidget_cellChanged(int row, int column)
{
	if (column == 0)
	{
		QTableWidgetItem* freqItem = ui->tableWidget->item(row, 0);
		bool ok;
		double freq = freqItem->text().toDouble(&ok);
		if (ok)
		{
			double gain = scene->getNodes()[row].dbGain;
			scene->setNode(row, freq, gain);
		}
		else
		{
			freq = scene->getNodes()[row].freq;
			freqItem->setText(QString("%0").arg(freq));
		}
	}
	else if (column == 1)
	{
		QTableWidgetItem* gainItem = ui->tableWidget->item(row, 1);
		bool ok;
		double gain = gainItem->text().toDouble(&ok);
		if (ok)
		{
			double freq = scene->getNodes()[row].freq;
			scene->setNode(row, freq, gain);
		}
		else
		{
			gain = scene->getNodes()[row].dbGain;
			gainItem->setText(QString("%0").arg(gain));
		}
	}
}

void GraphicEQFilterGUI::on_tableWidget_itemSelectionChanged()
{
	QSet<int> selectedRows;
	for (QTableWidgetItem* item : ui->tableWidget->selectedItems())
		selectedRows.insert(item->row());

	scene->setSelectedNodes(selectedRows);
}

void GraphicEQFilterGUI::on_radioButton15_toggled(bool checked)
{
	if (checked)
	{
		scene->setBandCount(15);
		setFreqEditable(false);
	}
}

void GraphicEQFilterGUI::on_radioButton31_toggled(bool checked)
{
	if (checked)
	{
		scene->setBandCount(31);
		setFreqEditable(false);
	}
}

void GraphicEQFilterGUI::on_radioButtonVar_toggled(bool checked)
{
	if (checked)
	{
		scene->setBandCount(-1);
		setFreqEditable(true);
	}
}

void GraphicEQFilterGUI::setFreqEditable(bool editable)
{
	ui->tableWidget->blockSignals(true);
	for (int i = 0; i < ui->tableWidget->rowCount(); i++)
	{
		QTableWidgetItem* item = ui->tableWidget->item(i, 0);
		if (editable)
			item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsEnabled);
		else
			item->setFlags(item->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsEnabled));
	}
	ui->tableWidget->blockSignals(false);
}

void GraphicEQFilterGUI::on_actionImport_triggered()
{
	QFileInfo fileInfo(configPath);
	QFileDialog dialog(this, tr("Import frequency response"), fileInfo.absolutePath(), "*.csv");
	dialog.setFileMode(QFileDialog::ExistingFiles);
	QStringList nameFilters;
	nameFilters.append(tr("Frequency response (*.csv)"));
	nameFilters.append(tr("All files (*.*)"));
	dialog.setNameFilters(nameFilters);
	if (dialog.exec() == QDialog::Accepted)
	{
		vector<FilterNode> newNodes;

		for (QString path : dialog.selectedFiles())
		{
			QFile file(path);
			if (file.open(QFile::ReadOnly))
			{
				QTextStream stream(&file);
				while (!stream.atEnd())
				{
					QString text = stream.readLine();
					if (text.startsWith('*'))
						continue;

					if (!text.contains('.'))
						text = text.replace(',', '.');
					QRegularExpressionMatchIterator it = numberRegEx.globalMatch(text);
					while (it.hasNext())
					{
						QRegularExpressionMatch match = it.next();
						QString freqString = match.captured();
						bool ok;
						double freq = freqString.toDouble(&ok);
						if (ok && it.hasNext())
						{
							match = it.next();
							QString gainString = match.captured();
							double gain = gainString.toDouble(&ok);
							if (ok)
								newNodes.push_back(FilterNode(freq, gain));
						}
					}
				}
			}
		}
		sort(newNodes.begin(), newNodes.end());

		scene->setNodes(newNodes);

		int bandCount = scene->verifyBands(newNodes);
		if (bandCount != scene->getBandCount())
		{
			switch (bandCount)
			{
			case 15:
				ui->radioButton15->click();
				break;
			case 31:
				ui->radioButton31->click();
				break;
			default:
				ui->radioButtonVar->click();
			}
		}
		else
		{
			emit updateModel();
		}
	}
}

void GraphicEQFilterGUI::on_actionExport_triggered()
{
	QFileInfo fileInfo(configPath);
	QFileDialog dialog(this, tr("Export frequency response"), fileInfo.absolutePath(), "*.csv");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	QStringList nameFilters;
	nameFilters.append(tr("Frequency response (*.csv)"));
	nameFilters.append(tr("All files (*.*)"));
	dialog.setNameFilters(nameFilters);
	dialog.setDefaultSuffix(".csv");

	if (dialog.exec() == QDialog::Accepted)
	{
		QString savePath = dialog.selectedFiles().first();

		QFile file(savePath);
		if (file.open(QFile::WriteOnly | QFile::Truncate))
		{
			QTextStream stream(&file);
			for (FilterNode node : scene->getNodes())
			{
				stream << node.freq << '\t' << node.dbGain << '\n';
			}
			stream.flush();
		}
	}
}

void GraphicEQFilterGUI::on_actionInvertResponse_triggered()
{
	vector<FilterNode> newNodes = scene->getNodes();
	for (FilterNode& node : newNodes)
		node.dbGain = -node.dbGain;

	scene->setNodes(newNodes);
	emit updateModel();
}

void GraphicEQFilterGUI::on_actionNormalizeResponse_triggered()
{
	vector<FilterNode> newNodes = scene->getNodes();

	double maxGain = -DBL_MAX;
	for (FilterNode node : newNodes)
	{
		if (node.dbGain > maxGain)
			maxGain = node.dbGain;
	}

	if (maxGain != 0)
	{
		for (FilterNode& node : newNodes)
			node.dbGain -= maxGain;

		scene->setNodes(newNodes);
		emit updateModel();
	}
}

void GraphicEQFilterGUI::on_actionResetResponse_triggered()
{
	vector<FilterNode> newNodes = scene->getNodes();
	QSet<int> selectedIndices = scene->getSelectedIndices();

	int index = 0;
	for (FilterNode& node : newNodes)
	{
		if (selectedIndices.isEmpty() || selectedIndices.contains(index))
			node.dbGain = 0.0;
		index++;
	}

	scene->setNodes(newNodes);
	scene->setSelectedNodes(selectedIndices);
	emit updateModel();
}
