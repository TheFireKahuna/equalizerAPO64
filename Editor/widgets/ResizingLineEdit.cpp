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

#include "ResizingLineEdit.h"

ResizingLineEdit::ResizingLineEdit(QWidget* parent)
	: EscapableLineEdit(parent)
{
	connect(this, SIGNAL(textChanged(QString)), this, SLOT(readjustSize()));

	readjustSize();
}

ResizingLineEdit::ResizingLineEdit(const QString& text, bool forceWidth, QWidget* parent)
	: EscapableLineEdit(text, parent), forceWidth(forceWidth)
{
	connect(this, SIGNAL(textChanged(QString)), this, SLOT(readjustSize()));

	readjustSize();
}

QSize ResizingLineEdit::sizeHint() const
{
	QSize originalSize = QLineEdit::sizeHint();

	QFontMetrics metrics = fontMetrics();
	QSize contentSize(metrics.width(text()), metrics.height());
	QStyleOptionFrameV2 opt;
	initStyleOption(&opt);

	QSize size = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, contentSize, this);
	return QSize(size.width(), originalSize.height());
}

void ResizingLineEdit::readjustSize()
{
	if (forceWidth)
		setFixedWidth(sizeHint().width());
	else
		setMinimumWidth(sizeHint().width());
}
