#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>
#include "code/CodeAnalyzer.h"
#include"gui/NameMapView.h"
// libclang 头文件
#include <clang-c/Index.h>
#include<qfile.h>
#include<qmessagebox.h>
#include<qlist.h>
#include<qvector.h>
#include<vector>
#include"code/CodeFileReader.h"
#include"code/CppCodeFileReader.h"
#include"MainClass.h"
#include"workFlow/PSCTranslater.h"
#include"workFlow/PSCInterpreter.h"
#include"CppFileAnalyzer.h"
//#include"CppTaskFactory.h"
//#include"CppOperator.h"
//#include"TaskManager.h"
//#include"baseOperator.h"
//#include"BaseOptFactory.h"
int main(int argc, char* argv[])
{
    //todo 添加一个new scope 操作


    
    QApplication app(argc, argv);
    //CppCodeAnalyzer COA;
    //auto res = COA.runAnalyzer("E:\\cpp\\qt\\CPHe\\code\\libClangContext.h");
    //res.print();
    PSCInterpreter it;
    it.runFile("E:\\cpp\\qt\\CPHe\\example.txt");

    return app.exec();
}