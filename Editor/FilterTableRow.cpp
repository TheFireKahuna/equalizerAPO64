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

#include <QToolBar>
#include <QLineEdit>
#include <QPainter>
#include <QScrollBar>

#include "Editor/helpers/DPIHelper.h"
#include "FilterTableRow.h"
#include "ui_FilterTableRow.h"

FilterTableRow::FilterTableRow(FilterTable* table, int number, FilterTable::Item* item, IFilterGUI* gui)
	: QWidget(table),
	ui(new Ui::FilterTableRow)
{
	ui->setupUi(this);
	ui->labelNumber->setMinimumWidth(DPIHelper::scale(25));

	this->table = table;
	this->item = item;
	this->gui = gui;

	ui->labelNumber->setText(QString("%0").arg(number));

	ui->toolBar->addAction(ui->actionAdd);
	ui->toolBar->addAction(ui->actionRemove);
	ui->toolBar->addAction(ui->actionEditText);
	ui->toolBar->updateMaximumHeight();

	ui->stackedWidget->setContentsMargins(0, 5, 0, 5);

	if (gui != NULL)
	{
		connect(gui, SIGNAL(updateModel()), this, SLOT(updateModel()));
		ui->stackedWidget->addWidget(gui);
	}
	else
	{
		ui->stackedWidget->addWidget(new QLabel(item->text));
	}
	ui->stackedWidget->setCurrentIndex(1);
}

FilterTableRow::~FilterTableRow()
{
	delete ui;
}

QRect FilterTableRow::getHeaderRect()
{
	QRect rect(0, 0, ui->labelNumber->geometry().right() + 1, height());

	return rect;
}

void FilterTableRow::editText()
{
	if (!ui->actionEditText->isChecked())
		ui->actionEditText->trigger();
}

QSize FilterTableRow::sizeHint() const
{
	QSize size = QWidget::minimumSizeHint();
	int preferredWidth = table->getPreferredWidth();
	if (size.width() < preferredWidth)
		size = QSize(preferredWidth, size.height());
	return size;
}

void FilterTableRow::mouseDoubleClickEvent(QMouseEvent*)
{
	if (gui == NULL && ui->stackedWidget->currentIndex() == 1)
		ui->actionEditText->trigger();
}

void FilterTableRow::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QRectF r = rect();
	r = r.marginsAdded(QMarginsF(-1.5, -1.5, -1.5, -0.5));

	QColor color;
	if (table->getSelectedItems().contains(item))
	{
		if (table->getFocusedItem() == item)
			color = QColor(44, 111, 222);
		else
			color = QColor(64, 136, 255);
	}
	else
	{
		if (table->getFocusedItem() == item)
			color = QColor(160, 160, 160);
		else
			color = QColor(180, 180, 180);
	}
	painter.setPen(color);

	QLinearGradient gradient(r.topLeft(), r.bottomLeft());
	gradient.setColorAt(0, QColor(255, 255, 255));
	gradient.setColorAt(1, QColor(230, 230, 230));
	painter.setBrush(gradient);
	painter.drawRoundedRect(r, 5, 5);

	QRectF r2 = r;
	r2.setRight(ui->labelNumber->geometry().right() - 0.5);

	QLinearGradient gradient2(r2.topLeft(), r2.bottomLeft());
	gradient2.setColorAt(0, color.darker(110));
	gradient2.setColorAt(1, color.darker(150));
	painter.setBrush(gradient2);
	painter.drawRoundedRect(r2, 5, 5);
}

void FilterTableRow::updateModel()
{
	IFilterGUI* sender = qobject_cast<IFilterGUI*>(QObject::sender());
	QString command;
	QString parameters;
	sender->store(command, parameters);

	item->text = command + ": " + parameters;

	qDebug("Updated item text to %s", item->text.toStdString().c_str());
}

void FilterTableRow::on_actionAdd_triggered()
{
	QMenu* menu = table->createAddPopupMenu();
	QRect rect = ui->toolBar->actionGeometry(ui->actionAdd);
	QPoint p = ui->toolBar->mapToGlobal(QPoint(rect.x(), rect.y() + rect.height()));
	QAction* action = menu->exec(p);
	ui->actionAdd->setChecked(false);
	if (action != NULL)
	{
		FilterTemplate t = action->data().value<FilterTemplate>();
		QString line = t.getLine();
		table->addLine(line, item);
		table->updateGuis();
	}
}

void FilterTableRow::on_actionRemove_triggered()
{
	table->removeItem(item);
	table->updateGuis();
}

void FilterTableRow::on_actionEditText_triggered(bool checked)
{
	if (checked)
	{
		if (!lastEditTime.isValid() || lastEditTime.elapsed() > 100)
		{
			ui->lineEdit->setText(item->text);
			ui->stackedWidget->setCurrentIndex(0);
			ui->lineEdit->setFocus();
		}
		else
		{
			ui->actionEditText->setChecked(false);
		}
	}
}

void FilterTableRow::on_lineEdit_editingFinished()
{
	if (ui->stackedWidget->currentIndex() == 0 && !editingDone)
	{
		if (ui->lineEdit->text() != item->text)
		{
			editingDone = true;
			item->text = ui->lineEdit->text();
			table->updateModel();
			// set focus to table so that enter key does not cause scrolling down
			table->setFocus();
			table->updateGuis();
		}
		else
		{
			ui->stackedWidget->setCurrentIndex(1);
			lastEditTime.start();
			ui->actionEditText->setChecked(false);
		}
	}
}

void FilterTableRow::on_lineEdit_editingCanceled()
{
	if (ui->stackedWidget->currentIndex() == 0)
	{
		// will cause editingFinished to be called, so prevent committing
		editingDone = true;
		table->setFocus();
		ui->stackedWidget->setCurrentIndex(1);
		editingDone = false;
		ui->actionEditText->setChecked(false);
	}
}
