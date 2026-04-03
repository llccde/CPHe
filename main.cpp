#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>
#include "code/CodeAnalyzer.h"

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
class QFileReader :public M_File {
    class My_Content :public CharWrapper {
        // 通过 CharWrapper 继承
        uniqueCharArray res;
    public:
        My_Content(my_size length,char*&& data):CharWrapper(length),res(data) {
        }
        const char* getCharArray() override{
            return res.get();
        }
        ~My_Content() {};
    };
private:
    QFile file;
public:
    QFileReader(QString path):file(path) {
        
    };
    ContentRes getContent() override
    {
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return nullptr;
        }
        QByteArray data = file.readAll();
        my_size size = data.size() + 1;
        char* buffer = new char[size];
        strcpy(buffer,data.constData());

        file.close();
        return ContentRes(
            new My_Content(size,std::move(buffer))
        );
    }
    QString getFullPath() override
    {
        return QFileInfo(file).absoluteFilePath();
    }
};



int main(int argc, char* argv[])
{
    
    
    auto a = CodeAnalyzer(std::make_unique<LibClangContext>());
    a.getLibClangContext()->addFile(UniqueFilePtr(new QFileReader("E:\\cpp\\qt\\CPHe\\code\\testclass.cpp")),
        LibClangContext::isMainFile
    );
    std::unique_ptr<CPPCodeVisitor> visitor (new CPPCodeVisitor());
    a.launch(visitor.get());
    
    auto d = visitor->getNameMap()->findNodeByNameSpaceCallOnRoot("myName::myClass::func2");
    auto d2 = visitor->getNameMap()->findNodeByNameSpaceCallOnRoot("myName::myClass");
    for (auto &i:d)
    {
        CppCodeFileReader cfr(true, i->getPosition(), 4);
        for (size_t i = 0; i < cfr.getRowCount(); i++)
        {
            std::cout << cfr.readLine(i).toStdString() << "\n";
        }
        

    }
    QApplication app(argc, argv);
    
    

    return 0;
}