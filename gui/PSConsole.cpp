#include "PSConsole.h"

PSConsole::PSConsole(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::PSConsoleClass())
{
	ui->setupUi(this);
}

PSConsole::~PSConsole()
{
	delete ui;
}

