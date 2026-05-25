// AIWorkFlow.h
#pragma once
#include "ClangTool.h"
#include "AIClient.h"
#include "Interpreter.h"
#include <qdir.h>
#include"FileManager.h"
namespace awf {
    class AIWorkFlow;
}

class awf::AIWorkFlow {
public:
    Interpreter fileProcesser;
    ClangTool tool;
    AIClient aic;
    QString workingFolder = "";
    LineBaseFileManager fileManeger;
    bool doWrite = true;

    QString getAbsPath(const QString& path);
    AIWorkFlow(const QString& working);
    

    int index = -1;
    int genId = 0;

    QString readKey(QString key = "key.txt");
    QString getGenId();
    int getCurrentIndex();
    M_Command getCurrentCommand();
    QString currentSource();
    M_Command peekNext();
    void next();
    bool hasNext();

    void replaceWithFileBufferAndBackup(const QString& filePath, const QString& outPath);

    bool isCurInCommentBlock();
    void writeFile(const QString& data, int index = -1);
    void writeFile(const QVector<QString>& data, int index = -1);
    void writeComment(const QString& data);
    void writeComment(const QVector<QString> data);
    void writeSource(const QString& data);
    void writeSource(const QVector<QString> data);
    

    ExceptionCollector ec;
    void riseWarn(const QString& wrn);
    void riseError(const QString& err);
    bool hasError();
    void writeCurrentSource();

    M_Command justNextCommand();

    void newFileCommand();
    void debugger();
    void handleGenLable();
    void fillCommand();
    QString handleRef();
    inline void setWrite(bool v) {
        doWrite = v;
    }
    void launch(const QString& filePath, const QString& outPath);
    void prepareLaunch(const QString& filePath, const QString& outPath, int beginRow = -1);
};