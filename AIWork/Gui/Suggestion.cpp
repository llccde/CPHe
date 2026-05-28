#include "Suggestion.h"
#include"qdir.h"



using namespace awf;
static QVector<QString> filterStartingWith(const QVector<QString>& strings, const QString& prefix) {
    QVector<QString> result;
    for (const QString& s : strings) {
        if (s.startsWith(prefix, Qt::CaseInsensitive)) {
            result.append(s);
        }
    }
    return result;
}
// ---------- 构造函数（原代码保持不变） ----------
awf::Suggestion::Suggestion(const QString& working) : workingFolder(working)
    ,
    noSuggestion_([](const QString&) -> QVector<QString> { return {}; })
    ,
    completeFilePath_([this](const QString& partial) -> QVector<QString> {
        // 1. 分离目录部分与文件名前缀
        QString dirPart;
        QString prefix;
        // 统一处理 '/' 和 '\'
        int lastSlash = std::max(partial.lastIndexOf(QLatin1Char('/')),
            partial.lastIndexOf(QLatin1Char('\\')));
        if (lastSlash != -1) {
            dirPart = partial.left(lastSlash);       // 如 "text"
            prefix = partial.mid(lastSlash + 1);    // 如 "a"
        }
        else {
            // 没有路径分隔符，整个就是前缀
            prefix = partial;
        }

        // 2. 构建目标目录路径
        QString targetDir = workingFolder;
        if (!dirPart.isEmpty()) {
            targetDir += QDir::separator() + dirPart;
        }

        // 3. 检查目录是否存在
        QDir dir(targetDir);
        if (!dir.exists()) {
            return {};
        }

        // 4. 列出所有以 prefix 开头的文件和文件夹
        QStringList nameFilters;
        nameFilters << prefix + QStringLiteral("*");
        QStringList entries = dir.entryList(nameFilters,
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
            QDir::Name);
        // 转换为 QVector<QString> 返回
        return QVector<QString>(entries.begin(), entries.end());
    }),
    completeSymbol_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
    ,
    completeSymbolName_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
    ,
    completeModelName_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
    ,
    completeBoolean_([this](const QString& partial) -> QVector<QString> {
    return {"true","false"};
        })
    ,
    completeId_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
    ,
    completeMessage_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
    ,
    completeSingleArg_([this](const QString& partial) -> QVector<QString> {
    return {};
        })
{
    sugtable = {
        { MP::fill, {
            { Args::single,    completeSingleArg_ },
            { Args::symbolName, completeSymbolName_ },
            { Args::modelName,  completeModelName_ },
            { Args::id,        completeId_ }
        }},
        { MP::genBegin, {
            { Args::id,        completeId_ },
            { Args::symbolName, completeSymbolName_ },
            { Args::modelName,  completeModelName_ }
        }},

        { MP::genEnd, {
            { Args::id,        completeId_ }
        }},
        { MP::ref, {
            { Args::single,    completeSingleArg_ },
            { Args::file,      completeFilePath_ },
            { Args::callLLM,   completeBoolean_ },
            { Args::cache,     completeBoolean_ },
            { Args::symbol,    completeSymbol_ },
            { Args::msg,       completeMessage_ }
        }},
        { MP::record, {
            { Args::symbol,    completeSymbol_ },
            { Args::id,        completeId_ }
        }},
        { MP::recordEnd, {
            { Args::id,        completeId_ }
        }},
        { MP::end, {
            { Args::id,        completeId_ }
        }},        
        { MP::msg,{}},
        { MP::normalComment, {}},
        { MP::notCommand, {}},
        { MP::debugger, {}},
        { MP::nameFunc, {}},
        { MP::moduleName, {}},
        { MP::copyPrompt, {}},
    };
}

// ---------- 补全方法实现 ----------

QVector<QString> awf::Suggestion::getOperator(QString opPart) {
    QVector<QString> opStrings;
    const auto ops = sugtable.keys();   // 获取所有已配置的指令类型
    for (const auto& op : ops) {
        opStrings.append(operatorTypeToString(op));
    }
    return filterStartingWith(opStrings, opPart);
}

QVector<QString> awf::Suggestion::getArgs(QString op, QString argKeyPart) {
    M_OperatorType opType = stringToOperatorType(op);
    if (!sugtable.contains(opType)) {
        return {};
    }
    const auto& argMap = sugtable[opType];
    QVector<QString> argStrings;
    for (auto it = argMap.keyBegin(); it != argMap.keyEnd(); ++it) {
        argStrings.append(ArgsClass::toString(*it));
    }
    return filterStartingWith(argStrings, argKeyPart);
}

QVector<QString> awf::Suggestion::getArgValue(QString op, QString argKey, QString argValPart) {
    M_OperatorType opType = stringToOperatorType(op);
    Args argEnum = ArgsClass::fromString(argKey);
    if (sugtable.contains(opType) && sugtable[opType].contains(argEnum)) {
        Handler handler = sugtable[opType][argEnum];
        return handler(argValPart);                     // 直接调用对应补全函数并返回结果
    }
    return {};                                          // 无匹配时返回空列表
}
