#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>
#include "code/CodeAnalyzer.h"
#include"gui/NameMapView.h"
// libclang 头文件
#include <clang-c/Index.h>
#include <iostream>
#include<qfile.h>
#include<qmessagebox.h>
#include<qlist.h>
#include<qvector.h>
#include<vector>
#include"code/CodeFileReader.h"
#include"code/CppCodeFileReader.h"
#include"libClangContext.h"
#include"NameTree.h"
#include"CppCodeVisitor.h"
#include"QFileReader.h"
#include"MainClass.h"
#include"workFlow/PSCTranslater.h"
//#include"CppTaskFactory.h"
//#include"CppOperator.h"
//#include"TaskManager.h"
//#include"baseOperator.h"
//#include"BaseOptFactory.h"
int main(int argc, char* argv[])
{
    //todo 添加一个new scope 操作


    
    QApplication app(argc, argv);
    //TaskManager tasks;
    //tasks.registTaskFactory(std::unique_ptr<TaskFactory>(new CppTaskFactory()));
    //tasks.registTaskFactory(std::unique_ptr<TaskFactory>(new BaseOptFactory()));

    //auto s = tasks.getAllOperator();
    //auto thTask1 = tasks.prepareOperator("newScope");
    //thTask1->launch();
    //auto thTask2 = tasks.prepareOperator("loadCppFile");
    //thTask2->AddIntent("E:\\cpp\\qt\\CPHe\\workFlow\\forCpp\\CppContext.h");
    //thTask2->launch();
    //auto task3 = tasks.prepareOperator("setCppMainFile");
    //task3->AddIntent("CppContext.h");
    //task3->launch();
    //auto task4 = tasks.prepareOperator("RunCppAnalyze");
    //task4->launch();

    /*MainClass mainClass;
    mainClass.showAll();*/

    PSCTranslater parser;
    PSCCommand cmd;

    // 完整形式
    cmd = parser.translate("@def ins:main");
    cmd.output();
    parser.outputErr();
    // 省略 recv，直接跟冒号
    cmd = parser.translate("@depDecl:main");
    cmd.output();
    parser.outputErr();
    // 多参数，带转义
    cmd = parser.translate("@decl ins:ClassA::InnerA\\;ClassB");
    cmd.output();
    parser.outputErr();
    // 没有参数
    cmd = parser.translate("@set file:");
    cmd.output();
    parser.outputErr();
    // 错误输入
    cmd = parser.translate("@3abc");
    cmd.output();
    parser.outputErr();
    cmd = parser.translate("@def recv(hello):\\@main(hell)");
    cmd.output();
    parser.outputErr();


    return app.exec();
}