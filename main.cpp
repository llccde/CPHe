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

int main(int argc, char* argv[])
{




    QApplication app(argc, argv);
    MainClass mainClass;
    mainClass.showAll();

    return 0;//app.exec();
}