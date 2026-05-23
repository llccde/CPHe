// AIWorkFlow.h
#pragma once
#include "ClangTool.h"
#include "AIClient.h"
#include "Interpreter.h"
#include <qdir.h>

namespace awf {
    class AIWorkFlow;
}

class awf::AIWorkFlow {
public:
    Interpreter fileProcesser;
    ClangTool tool;
    AIClient aic;
    QString workingFolder = "";
    bool inCommentBlock = false;

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
    void next(bool write = true);
    bool hasNext();

    QString fileBuffer = "";
    void replaceWithFileBufferAndBackup(const QString& filePath, const QString& outPath);

    void writeFile(const QString& data, int index = -1);
    void writeComment(const QString& data, int index = -1);
    void writeComment(const QVector<QString> data, int index = -1);
    void writeSource(const QString& data, int index = -1);
    void writeSource(const QVector<QString> data, int index = -1);
    void writeFile(const QVector<QString>& data, int index = -1);

    ExceptionCollector ec;
    void riseWarn(const QString& wrn);
    void riseError(const QString& err);
    bool hasError();
    void writeCurrentSource();

    M_Command justNextCommand();
    M_Command skipNextCommand();

    void newFileCommand();
    void debugger();
    void handleGenLable();
    void fillCommand();
    QString handleRef();
    void launch(const QString& filePath, const QString& outPath);
};