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

#include <algorithm>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

#include "CopyFilterGUIRow.h"
#include "CopyFilterGUIForm.h"

using namespace std;

CopyFilterGUIForm::CopyFilterGUIForm(QWidget* parent)
	: QWidget(parent)
{
}

void CopyFilterGUIForm::load(vector<Assignment> assignments)
{
	if (gridLayout != NULL)
		delete gridLayout;

	for (QObject* object : children())
	{
		QWidget* widget = qobject_cast<QWidget*>(object);
		if (widget != NULL)
			widget->setVisible(false);
		object->deleteLater();
	}

	gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(2, 2, 2, 2);
	gridLayout->setHorizontalSpacing(2);
	gridLayout->setVerticalSpacing(2);
	gridLayout->setColumnStretch(4, 1);
	gridLayout->setColumnStretch(5, 1);

	int row = 0;
	for (Assignment assignment : assignments)
	{
		QString oc = QString::fromStdWString(assignment.targetChannel);

		bool firstSummand = true;
		for (Assignment::Summand summand : assignment.sourceSum)
		{
			if (firstSummand)
			{
				gridLayout->addWidget(new QLabel(tr("Channel"), this), row, 0);

				QComboBox* targetComboBox = new QComboBox(this);
				targetComboBox->setEditable(true);
				for (wstring channelName : inputChannelNames)
					targetComboBox->addItem(QString::fromStdWString(channelName));
				targetComboBox->setEditText(oc);
				connect(targetComboBox, SIGNAL(editTextChanged(QString)), this, SIGNAL(updateModel()));
				connect(targetComboBox, SIGNAL(editTextChanged(QString)), this, SIGNAL(updateChannels()));
				gridLayout->addWidget(targetComboBox, row, 1);

				gridLayout->addWidget(new QLabel("=", this), row, 2);

				firstSummand = false;
			}
			else
			{
				gridLayout->addWidget(new QLabel("+", this), row, 2);
			}

			CopyFilterGUIRow* rowWidget = new CopyFilterGUIRow(summand, inputChannelNames, this);
			connect(rowWidget, SIGNAL(updateModel()), this, SIGNAL(updateModel()));
			connect(rowWidget, SIGNAL(updateChannels()), this, SIGNAL(updateChannels()));
			gridLayout->addWidget(rowWidget, row, 3);

			QPushButton* addSummandButton = new QPushButton(this);
			addSummandButton->setText(tr("Add summand"));
			addSummandButton->setIcon(QIcon(":/icons/list-add-green.ico"));
			gridLayout->addWidget(addSummandButton, row, 4);
			connect(addSummandButton, SIGNAL(pressed()), this, SLOT(addSummand()));

			QPushButton* removeButton = new QPushButton(this);
			if (assignment.sourceSum.size() == 1)
				removeButton->setText(tr("Remove assignment"));
			else
				removeButton->setText(tr("Remove summand"));
			removeButton->setIcon(QIcon(":/icons/list-remove-red.ico"));
			gridLayout->addWidget(removeButton, row, 5);
			connect(removeButton, SIGNAL(pressed()), this, SLOT(remove()));

			QSpacerItem* spacerItem = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);
			gridLayout->addItem(spacerItem, row, 6);

			row++;
		}
	}

	QPushButton* addAssignmentButton = new QPushButton(this);
	addAssignmentButton->setText(tr("Add assignment"));
	addAssignmentButton->setIcon(QIcon(":/icons/list-add-green.ico"));
	gridLayout->addWidget(addAssignmentButton, row++, 0, 1, 4);
	connect(addAssignmentButton, SIGNAL(pressed()), this, SLOT(addAssignment()));
}

void CopyFilterGUIForm::setChannelNames(const vector<wstring>& channelNames)
{
	inputChannelNames = channelNames;

	for (int row = 0; row < gridLayout->rowCount() - 1; row++)
	{
		QLayoutItem* item = gridLayout->itemAtPosition(row, 1);
		if (item != NULL)
		{
			QComboBox* targetComboBox = qobject_cast<QComboBox*>(item->widget());
			targetComboBox->blockSignals(true);
			QString text = targetComboBox->currentText();
			targetComboBox->clear();
			for (wstring channelName : channelNames)
				targetComboBox->addItem(QString::fromStdWString(channelName));
			targetComboBox->setEditText(text);
			targetComboBox->blockSignals(false);
		}

		QLayoutItem* summandItem = gridLayout->itemAtPosition(row, 3);
		CopyFilterGUIRow* summandWidget = qobject_cast<CopyFilterGUIRow*>(summandItem->widget());
		summandWidget->setChannelNames(channelNames);
	}
}

vector<Assignment> CopyFilterGUIForm::buildAssignments(QWidget* pressedButton)
{
	vector<Assignment> assignments;

	Assignment* currentAssignment = NULL;
	for (int row = 0; row < gridLayout->rowCount() - 1; row++)
	{
		QLayoutItem* item = gridLayout->itemAtPosition(row, 1);
		if (item != NULL)
		{
			QComboBox* targetComboBox = qobject_cast<QComboBox*>(item->widget());
			QString oc = targetComboBox->currentText().trimmed();
			assignments.push_back(Assignment());
			currentAssignment = &assignments.back();
			currentAssignment->targetChannel = oc.toStdWString();
		}

		if (currentAssignment != NULL)
		{
			if (pressedButton != NULL)
			{
				QWidget* removeButton = gridLayout->itemAtPosition(row, 5)->widget();
				if (removeButton == pressedButton)
				{
					continue;
				}
			}

			QLayoutItem* summandItem = gridLayout->itemAtPosition(row, 3);
			CopyFilterGUIRow* summandWidget = qobject_cast<CopyFilterGUIRow*>(summandItem->widget());
			currentAssignment->sourceSum.push_back(summandWidget->buildSummand());

			if (pressedButton != NULL)
			{
				QWidget* addSummandButton = gridLayout->itemAtPosition(row, 4)->widget();
				if (addSummandButton == pressedButton)
				{
					Assignment::Summand newSummand;
					newSummand.channel = L" ";
					newSummand.factor = 1.0;
					newSummand.isDecibel = false;
					currentAssignment->sourceSum.push_back(newSummand);
				}
			}
		}
	}

	// remove empty assignments (can be caused by remove button)
	assignments.erase(remove_if(assignments.begin(), assignments.end(), [](Assignment assignment) {
		return assignment.sourceSum.empty();
	}), assignments.end());

	return assignments;
}

void CopyFilterGUIForm::addSummand()
{
	QWidget* addSummandButton = qobject_cast<QWidget*>(sender());

	vector<Assignment> assignments = buildAssignments(addSummandButton);

	load(assignments);
}

void CopyFilterGUIForm::remove()
{
	QWidget* removeButton = qobject_cast<QWidget*>(sender());

	vector<Assignment> assignments = buildAssignments(removeButton);

	load(assignments);

	emit updateModel();
	emit updateChannels();
}

void CopyFilterGUIForm::addAssignment()
{
	vector<Assignment> assignments = buildAssignments();

	Assignment newAssignment;
	Assignment::Summand summand;
	summand.channel = L" ";
	summand.factor = 1.0;
	summand.isDecibel = false;
	newAssignment.sourceSum.push_back(summand);

	assignments.push_back(newAssignment);

	load(assignments);
}
