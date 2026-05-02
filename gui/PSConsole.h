#pragma once

#include <QWidget>
#include "ui_PSConsole.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PSConsoleClass; };
QT_END_NAMESPACE

class PSConsole : public QWidget
{
	Q_OBJECT

public:
	PSConsole(QWidget *parent = nullptr);
	~PSConsole();

private:
	Ui::PSConsoleClass *ui;
};

