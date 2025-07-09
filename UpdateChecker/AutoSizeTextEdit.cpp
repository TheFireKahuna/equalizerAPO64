#include "stdafx.h"
#include "AutoSizeTextEdit.h"

AutoSizeTextEdit::AutoSizeTextEdit(QWidget* parent)
	: QTextEdit(parent)
{
}

AutoSizeTextEdit::~AutoSizeTextEdit()
{
}

QSize AutoSizeTextEdit::sizeHint() const
{
	QSize s(document()->size().toSize());
	return QSize(s.width() + contentsMargins().left() + contentsMargins().right(), s.height() + contentsMargins().top() + contentsMargins().bottom());
}
