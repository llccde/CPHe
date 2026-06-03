//#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>
//
//#include"AIWork/AIWorkFlow.h"
//#include"AIWork/Gui/DSLEditor.h"
//#include"AIWork/Gui/TheMainWindow.h"
//#include"AIWork/Gui/EditorsTabView.h"
//#include"AIWork/Gui/FileView.h"
//#include"AIWork/Gui/OutPutView.h"
//#include<qdebug.h>
//#include<iostream>
//#include"AIWork/BaseTool.h"
//#include <QFile>
//#include <QTextStream>
//#include <QString>
//#include<qfiledialog.h>
//#include"qobject.h"
//#include"qfileinfo.h"
//#include"qset.h"
//#include<qmessagebox.h>
//#include<qdir.h>
//#include<qsettings.h>
//#include<qthread.h>
//#include<functional>
//#include"AIWork/Workbench.h"
#include"RCTool/RecordCollector.h"
#include"RCTool/Analyzer.h"
#include<Windows.h>
int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    QApplication app(argc, argv);
    Analyzer ana(argc,argv);
    ana.work();
    if (ana.ec.hasSometing())ana.ec.printAll();
    //Workbench bench(app);   // 在构造函数中完成全部初始化
    return 0;
}