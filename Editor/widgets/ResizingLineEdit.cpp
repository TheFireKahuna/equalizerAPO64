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

	// from https://stackoverflow.com/a/73663065
	QFontMetrics metrics = fontMetrics();
	QSize textSize(metrics.size(0, text()));
	QMargins tm = textMargins();
	QSize textMarginSize(tm.left() + tm.right(), tm.top() + tm.bottom());
	QMargins cm = contentsMargins();
	QSize contentsMarginSize = QSize(cm.left() + cm.right(), cm.top() + cm.bottom());
	QSize extraSize(8, 4); // hard coded stuff in Qt
	QSize contentSize = textSize + textMarginSize + contentsMarginSize + extraSize;
	QStyleOptionFrame opt;
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
