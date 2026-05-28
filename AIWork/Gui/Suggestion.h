#pragma once

#include <QMap>
#include <QString>
#include <functional>
#include "../M_Command.h"

namespace awf {

    class Suggestion {
        using Handler = ::std::function<QVector<QString>(const QString&)>;
        QString workingFolder;
        // 将原来的成员函数替换为捕获 this 的 lambda 成员变量
        Handler noSuggestion_;
        Handler completeFilePath_;
        Handler completeSymbol_;
        Handler completeSymbolName_;
        Handler completeModelName_;
        Handler completeBoolean_;
        Handler completeId_;
        Handler completeMessage_;
        Handler completeSingleArg_;
        QMap<M_OperatorType, QMap<Args, Handler>> sugtable;
    public:
        explicit Suggestion(const QString& working = QString());
        QVector<QString> getOperator(QString opPart);
        QVector<QString> getArgs(QString op, QString argKeyPart);
        QVector<QString> getArgValue(QString op, QString argKey, QString argValPart);
    };

};// namespace awf