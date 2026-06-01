#pragma once
#include "ClangTool.h"
#include "AIClient.h"
#include "Interpreter.h"
#include "FileManager.h"
#include <qdir.h>
#include <qobject.h>
#include <QStack>
#include <memory>
namespace awf {
    class RuntimeCommand;
    class AIWorkFlow;

    // ---------- 文件内容缓冲 ----------
    struct FileBuffer {
        QString absPath;
        QString content;
    };

    class AIWorkFlow : public QObject {
        Q_OBJECT
    public:
    signals:
        void outPut(const QString& data);

    public:
        Interpreter fileProcesser;
        ClangTool tool;
        AIClient aic;
        QString workingFolder = "";
        LineBaseFileManager fileManeger;
        bool doWrite = true;

        QString getAbsPath(const QString& path);
        QString getRelativePath(const QString& path);
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

        // 保留根节点特殊指令处理函数
        void debugger();
        void handleGenLable();

        // 数据获取函数（只返回数据，不与栈交互）
        QString handleRef();
        QVector<FileBuffer> handleRefFile();

        inline void setWrite(bool v) { doWrite = v; }

        void launch(const QString& filePath, const QString& outPath);
        void prepareLaunch(const QString& filePath, const QString& outPath, int beginRow = -1);

        // 命令栈
        QStack<std::unique_ptr<RuntimeCommand>> commandStack;

        // 新主循环（替换原 newFileCommand）
        void newMainLoop();
    };

} // namespace awf