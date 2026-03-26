#include "CPHeMain.h"

CPHeMain::CPHeMain(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CPHeMainClass())
{
    ui->setupUi(this);
}

CPHeMain::~CPHeMain()
{
    delete ui;
}

