#include "PSCMainWindow.h"
#include"workFlowContext.h"
#include"PSConsole.h"
#include"qdockwidget.h"

#include"BaseSubContext.h"
PSCMainWindow::PSCMainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::PSCMainWindowClass())
{
	pscConsoleDock = new QDockWidget(this);
	pscConsole = new PSConsole(pscConsoleDock);
	pscConsoleDock->setWidget(pscConsole);
	this->addDockWidget(Qt::LeftDockWidgetArea, pscConsoleDock);
	ui->setupUi(this);
	
}

PSCMainWindow::~PSCMainWindow()
{
	delete ui;
}

