#ifndef PSCTRANSLATER_H
#define PSCTRANSLATER_H

#include <QString>
#include <QVector>
#include <QDebug>
#include "SubStr.h"
#include "PSCLangErr.h"
#include "PSCErrorType.h"

#include <QObject> 
// ---------- 最终数据结构 ----------
struct PSCCommandArg {
    bool valid = false;
    enum Type { rawStr, call } type = rawStr;
    QString body;          // rawStr: 内容；call: 接收者标识符
    QString argForCall;    // 仅 call 有效
    QString toString() const {
        switch (type) {
        case rawStr: return "str:\"" + body + "\"";
        case call:   return "@" + body + "(" + argForCall + ")";
        }
        return {};
    }

    PSCCommandArg(bool valid, const Type& type, const QString& body, const QString& argForCall)
        : valid(valid), type(type), body(body), argForCall(argForCall)
    {
    }

    PSCCommandArg() = default;
};

struct PSCCommand {
    bool valid = false;
    QString op;
    QString recv;
    QString recvUDI;
    QVector<PSCCommandArg> args;
    QString toString() const {
        QString s;
        QTextStream ts(&s);
        ts << "Command{ valid=" << (valid ? "true" : "false");
        if (!op.isEmpty())   ts << ", op=\"" << op << "\"";
        if (!recv.isEmpty())
        {
            ts << ", recv=\"" << recv;
            if (!recvUDI.isEmpty()) {
                ts << "(" << recvUDI << ")";
            }
            ts << "\"";
        }
        ts << ", args=[";
        for (int i = 0; i < args.size(); ++i) {
            if (i > 0) ts << ", ";
            ts << "" << args[i].toString() << "";
        }
        ts << "] }";
        return s;
    }
    void output() const { qDebug().noquote() << toString(); }

    PSCCommand(bool valid, const QString& op, const QString& recv, const QString& recvUDI, const QVector<PSCCommandArg>& args)
        : valid(valid), op(op), recv(recv), recvUDI(recvUDI), args(args)
    {
    }

    PSCCommand() = default;
};

// ---------- 解析中间结果 ----------
enum class PSCParseState {
    Success,
    Error,
    Empty
};



struct MatchResult {
    PSCParseState state = PSCParseState::Error;
    SubStr result;       // 匹配到的子串视图
};

struct LeftResult {
    PSCParseState state = PSCParseState::Error;
    SubStr op;
    SubStr recvName;
    SubStr userDefID;
};

struct RecvResult {
    PSCParseState state = PSCParseState::Error;
    SubStr recvName;
    SubStr userDefID;
};

struct ArgResult {
    PSCParseState state = PSCParseState::Error;
    PSCCommandArg arg;
};

struct ArgsResult {
    PSCParseState state = PSCParseState::Error;
    QVector<PSCCommandArg> args;
};

// ---------- 主类 ----------
class PSCTranslater {
public:
    PSCTranslater() = default;

    // 主要接口：翻译原始字符串，错误由 getErr() 获取
    PSCCommand translate(const QString& raw);
    const QVector<PSCLangErr>& getErr() const { return m_errors; }
    void outputErr() const;
private:
    // 错误收集
    
    void collectError(const SubStr& where, PSCErrorType::errorType type);

    // 产生式函数（不再包含 errors 参数）
    MatchResult parseID(const SubStr& str, int& pos, PSCErrorType::errorType errorType, bool required);
    MatchResult parseOp(const SubStr& str, int& pos);
    MatchResult parseRecvName(const SubStr& str, int& pos);
    MatchResult parseUserDefID(const SubStr& str, int& pos);  // 允许 empty
    RecvResult parseHasRecv(const SubStr& str, int& pos);
    RecvResult parseRecv(const SubStr& str, int& pos);
    LeftResult parseLeft(const SubStr& leftStr);

    ArgResult parseCall(const SubStr& argStr);
    ArgResult parseRawStr(const SubStr& str);
    ArgResult parseArg(const SubStr& argStr);
    ArgsResult parseArgs(const SubStr& argsStr);

    // 纯工具：从 SubStr 切割未转义分号
    QVector<SubStr> splitArgs(const SubStr& argsStr) const;

    // 转义处理（应用于 rawStr 的 body）
    QString unescape(const QString& str) const;

    // 错误存储（每次 translate 前清空）
    QVector<PSCLangErr> m_errors;
    QString lastRaw = "";
};

#endif // PSCTRANSLATER_H