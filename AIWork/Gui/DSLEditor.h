#pragma once

#include <QWidget>
#include "ui_DSLEditor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DSLEditorClass; };
QT_END_NAMESPACE

class DSLEditor : public QWidget
{
	Q_OBJECT

public:
	DSLEditor(QWidget *parent = nullptr);
	~DSLEditor();

private:
	Ui::DSLEditorClass *ui;
};

