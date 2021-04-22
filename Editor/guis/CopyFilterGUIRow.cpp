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

#include "CopyFilterGUIRow.h"
#include "ui_CopyFilterGUIRow.h"

using namespace std;

CopyFilterGUIRow::CopyFilterGUIRow(Assignment::Summand summand, std::vector<wstring> channelNames, QWidget* parent)
	: QWidget(parent),
	ui(new Ui::CopyFilterGUIRow)
{
	ui->setupUi(this);

	ui->modeComboBox->addItem(tr("Channel"), 'c');
	ui->modeComboBox->addItem(tr("Factor * channel"), 'f');
	ui->modeComboBox->addItem(tr("dB factor * channel"), 'd');
	ui->modeComboBox->addItem(tr("Constant value"), 'v');

	QChar mode;
	if (summand.channel == L"")
		mode = 'v';
	else if (summand.isDecibel)
		mode = 'd';
	else if (summand.factor != 1.0)
		mode = 'f';
	else
		mode = 'c';

	ui->modeComboBox->setCurrentIndex(ui->modeComboBox->findData(mode));

	ui->factorSpinBox->setValue(summand.factor);

	for (wstring channelName : channelNames)
		ui->channelComboBox->addItem(QString::fromStdWString(channelName));
	ui->channelComboBox->setEditText(QString::fromStdWString(summand.channel).trimmed());

	connect(ui->modeComboBox, SIGNAL(activated(int)), this, SIGNAL(updateModel()));
	connect(ui->factorSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(updateModel()));
	connect(ui->channelComboBox, SIGNAL(editTextChanged(QString)), this, SIGNAL(updateModel()));
	connect(ui->channelComboBox, SIGNAL(editTextChanged(QString)), this, SIGNAL(updateChannels()));
}

CopyFilterGUIRow::~CopyFilterGUIRow()
{
	delete ui;
}

void CopyFilterGUIRow::setChannelNames(const vector<wstring>& channelNames)
{
	ui->channelComboBox->blockSignals(true);
	QString text = ui->channelComboBox->currentText();
	ui->channelComboBox->clear();
	for (wstring channelName : channelNames)
		ui->channelComboBox->addItem(QString::fromStdWString(channelName));
	ui->channelComboBox->setEditText(text);
	ui->channelComboBox->blockSignals(false);
}

Assignment::Summand CopyFilterGUIRow::buildSummand()
{
	Assignment::Summand summand;

	QChar mode = ui->modeComboBox->currentData().toChar();
	if (mode != 'c')
		summand.factor = ui->factorSpinBox->value();
	else
		summand.factor = 1.0;
	// fix for rounding error when using the spin buttons
	if (abs(summand.factor) < 1e-10)
		summand.factor = 0.0;

	if (mode != 'v')
		summand.channel = ui->channelComboBox->currentText().trimmed().toStdWString();
	// allow channel mode to be remembered when reloading
	if (mode == 'c' && summand.channel == L"")
		summand.channel = L" ";

	summand.isDecibel = mode == 'd';

	return summand;
}

void CopyFilterGUIRow::on_modeComboBox_currentIndexChanged(int index)
{
	QChar mode = ui->modeComboBox->itemData(index).toChar();
	ui->factorSpinBox->setVisible(mode != 'c');
	ui->asteriskLabel->setVisible(mode == 'f' || mode == 'd');
	ui->channelComboBox->setVisible(mode != 'v');
	ui->factorSpinBox->setSuffix(mode == 'd' ? " dB" : "");
}
