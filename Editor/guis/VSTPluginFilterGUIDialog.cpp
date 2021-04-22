/*
    This file is part of Equalizer APO, a system-wide equalizer.
    Copyright (C) 2017  Jonas Thedering

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

#include <QPushButton>
#include "VSTPluginFilterGUIDialog.h"
#include "ui_VSTPluginFilterGUIDialog.h"

using namespace std;
using namespace std::placeholders;

VSTPluginFilterGUIDialog::VSTPluginFilterGUIDialog(QWidget* parent, VSTPluginInstance* effect, bool autoApply)
	: QDialog(parent, Qt::WindowCloseButtonHint),
	ui(new Ui::VSTPluginFilterGUIDialog),
	effect(effect)
{
	ui->setupUi(this);
	ui->autoApplyCheckBox->setChecked(autoApply);

	QString name = QString::fromStdWString(effect->getName());
	setWindowTitle(name);

	HWND hwnd = (HWND)ui->frame->winId();

	short width, height;

	effect->startEditing(hwnd, &width, &height);

	ui->frame->setFixedSize(width, height);
	effect->setSizeWindowFunc(bind(&VSTPluginFilterGUIDialog::onSizeWindow, this, _1, _2));
}

VSTPluginFilterGUIDialog::~VSTPluginFilterGUIDialog()
{
	effect->stopEditing();
	effect->setSizeWindowFunc(nullptr);

	delete ui;
}

QPushButton* VSTPluginFilterGUIDialog::getApplyButton()
{
	return ui->buttonBox->button(QDialogButtonBox::Apply);
}

QCheckBox* VSTPluginFilterGUIDialog::getAutoApplyCheckBox()
{
	return ui->autoApplyCheckBox;
}

void VSTPluginFilterGUIDialog::onSizeWindow(int w, int h)
{
	ui->frame->setFixedSize(w, h);
}

void VSTPluginFilterGUIDialog::on_autoApplyCheckBox_clicked(bool checked)
{
	if (checked)
		getApplyButton()->click();
}
