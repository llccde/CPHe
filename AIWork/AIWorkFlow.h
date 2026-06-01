// AIWorkFlow.h
#pragma once
#include "ClangTool.h"
#include "AIClient.h"
#include "Interpreter.h"
#include <qdir.h>
#include"FileManager.h"
#include<qobject.h>
#include<memory>


namespace awf {
    class AIWorkFlow;
    struct FileBuffer {
        QString absPath;
        QString content;
    };
    enum SymbolType {
        Class,Func,Identify
    };
    class RuntimeCommand {
        #define NO_IMPLEMENT ec->riseErr("no such method:" + QString(__func__));
    public:
        AIWorkFlow* aiwf;
        ExceptionCollector* ec;
        M_Command command;
    public:
        virtual void onLaunch() { NO_IMPLEMENT }
        virtual void onFinish() { NO_IMPLEMENT }
        virtual void onReciveFileContent(QString content, QString path,Role role = system) {NO_IMPLEMENT};
        virtual void onName(SymbolType type, QString name) {NO_IMPLEMENT}
    };
    class BasePromopt :public RuntimeCommand {
        QVector<ChatMessage> promote;
    public:
        void onReciveFileContent(QString content, QString pathn, Role role = system) override {
            //指令不一定听从参数
            switch (aiwf->getCurrentCommand().type)
            {
            case MP::ref:
            case MP::refFiles:
                promote.append({ system,content });
            default:
                promote.append({ role,content });
                break;
            }
        }
    };
    class FillCommand:public BasePromopt{
    public:
        void onName(SymbolType type, QString name) override {
        }
        void onLaunch() {
            //do some
        };
        void onFinish() {
            //do dome
        };
    };
    class PrintCommand :public BasePromopt {
    public:
        void onLaunch() {
            //do some
        };
        void onFinish() {
            //do dome
        };
    };
    class ChatCommand :public BasePromopt {
    public:
        void onLaunch() {
            //do some
        };
        void onFinish() {
            //do dome
        };
    };
}





class awf::AIWorkFlow :public QObject{
    Q_OBJECT;
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

    void newFileCommand();
    void debugger();
    void handleGenLable();
    void fillCommand();
    void chatCommand();
    void printCommand();
    QString handleRef();
    QVector<FileBuffer> handleRefFile();
    inline void setWrite(bool v) {
        doWrite = v;
    }
    void launch(const QString& filePath, const QString& outPath);
    void prepareLaunch(const QString& filePath, const QString& outPath, int beginRow = -1);

    QStack<std::unique_ptr<RuntimeCommand>> commandStack;
    void newMainLoop() {
    
        while (hasNext()&&!ec.hasErr())
        {
            next();
            auto cur = getCurrentCommand();
            if (cur.isLongOperator) {
                RuntimeCommand* rc;
                switch (cur.type)
                {
                case MP::fill:
                    rc = new FillCommand();
                case MP::print:
                    rc = new PrintCommand();
                case MP::chat:
                    rc = new ChatCommand();
                default:
                    rc = new BasePromopt();
                    ec.riseErr(QString("不支持的 长指令 类型 %1").arg(awf::operatorTypeToString(cur.type)));
                    
                    break;
                }
                rc->command = cur;
                rc->aiwf = this;
                rc->ec = &ec;
                rc->onLaunch();
                commandStack.push(std::make_unique<RuntimeCommand>(rc));
            }
            else
            {

                switch (cur.type)
                {
                case MP::ref:
                    handleRefFile();
                case MP::end:
                    if (!commandStack.empty()) {
                        commandStack.top()->onFinish();
                        commandStack.pop();
                    }
                    else
                    {
                        riseError("未匹配的 @end");
                    }
                //其他指令的case;
                default:
                    break;
                }
            }
        }
        if (!commandStack.empty()) {
            QVector<QString> names;
            while (!commandStack.empty());
            {
                names.append(awf::operatorTypeToString(commandStack.pop()->command.type));
            }
            riseError(QString("缺少结尾的长指令 :\n%1").arg(names.join("\n")));
        }
    }
    void newHandleRef() {
        if (commandStack.empty()) {
            riseError("根节点下 不支持 @ref");
            return;
        }
        QString path;
        QString content;
        //各种逻辑

        //不关心栈顶指令是否实现了这个方法,确保指令正确嵌套是软件使用者的责任;
        commandStack.top()->onReciveFileContent(path, content);
        
    
    }



};