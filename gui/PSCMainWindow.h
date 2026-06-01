#pragma once

#include <QMainWindow>
#include "ui_PSCMainWindow.h"
#include"memory.h"
class QDockWidget;
class WorkFlowContext;
class PSConsole;
QT_BEGIN_NAMESPACE
namespace Ui { class PSCMainWindowClass; };
QT_END_NAMESPACE

class PSCMainWindow : public QMainWindow
{
	Q_OBJECT

public:

	PSCMainWindow(QWidget *parent = nullptr);
	~PSCMainWindow();

private:
	std::unique_ptr<WorkFlowContext> workFlowContext;
	Ui::PSCMainWindowClass *ui;
	QDockWidget* pscConsoleDock;
	PSConsole* pscConsole;
};

