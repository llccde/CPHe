#pragma once
#include"libClangContext.h"

class QFileReader :public M_File {
    class My_Content :public CharWrapper {
        // 通过 CharWrapper 继承
        uniqueCharArray res;
    public:
        My_Content(my_size length, char*&& data) :CharWrapper(length), res(data) {
        }
        const char* getCharArray() override {
            return res.get();
        }
        ~My_Content() {};
    };
private:
    QFile file;
public:
    QFileReader(QString path) :file(path) {

    };
    ContentRes getContent() override
    {

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return nullptr;
        }
        QByteArray data = file.readAll();
        my_size size = data.size() + 1;
        char* buffer = new char[size];
        strcpy(buffer, data.constData());

        file.close();
        return ContentRes(
            new My_Content(size, std::move(buffer))
        );
    }
    QString getFullPath() override
    {
        return QFileInfo(file).absoluteFilePath();
    }
};

