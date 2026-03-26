#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_CPHeMain.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CPHeMainClass; };
QT_END_NAMESPACE

class CPHeMain : public QMainWindow
{
    Q_OBJECT

public:
    CPHeMain(QWidget *parent = nullptr);
    ~CPHeMain();

private:
    Ui::CPHeMainClass *ui;
};

