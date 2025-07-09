#pragma once

#include <QTextEdit>

class AutoSizeTextEdit : public QTextEdit
{
	Q_OBJECT

public:
	AutoSizeTextEdit(QWidget* parent);
	~AutoSizeTextEdit();

	QSize sizeHint() const override;
};
