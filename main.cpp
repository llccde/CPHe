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
    
    
    //std::unique_ptr<LibClangContext> cntext(new LibClangContext());
    //auto a = CodeAnalyzer(cntext.get());
    //cntext->addFile(UniqueFilePtr(new QFileReader("E:\\cpp\\qt\\CPHe\\code\\testclass.cpp")),
    //    LibClangContext::isMainFile
    //);
    //std::unique_ptr<CPPCodeVisitor> visitor (new CPPCodeVisitor());
    //a.launch(visitor.get());
    //
    //auto d = visitor->getNameMap()->findNodeByNameSpaceCallOnRoot("myName::myClass::func2");
    //auto d2 = visitor->getNameMap()->findNodeByNameSpaceCallOnRoot("myName::myClass");
    //for (auto &i:d)
    //{
    //    CppCodeFileReader cfr(true, i->getPosition(), 4);
    //    for (size_t i = 0; i < cfr.getRowCount(); i++)
    //    {
    //        std::cout << cfr.readLine(i).toStdString() << "\n";
    //    }
    //    

    //}
    QApplication app(argc, argv);
    MainClass mainClass;
    mainClass.showAll();

    return app.exec();
}