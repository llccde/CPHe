#include "PSCTranslater.h"
#include <QMetaEnum>
// ---------- 静态工具 ----------
static QString parseIdentifierRaw(const SubStr& str, int& pos) {
    int start = pos;
    if (pos >= str.size()) return {};
    QChar first = str.at(pos);
    if (!first.isLetter() && first != '_') return {};
    ++pos;
    while (pos < str.size()) {
        QChar c = str.at(pos);
        if (c.isLetterOrNumber() || c == '_') ++pos;
        else break;
    }
    return str.mid(start, pos - start).toString();
}

MatchResult PSCTranslater::parseID(const SubStr& str, int& pos,
    PSCErrorType::errorType errorType, bool required)
{
    MatchResult mr;
    SubStr remaining = str.mid(pos);          // 未解析部分
    int innerPos = 0;
    QString id = parseIdentifierRaw(remaining, innerPos);  // 静态工具函数

    if (!id.isEmpty()) {
        // 解析成功：记录结果并推进外部位置
        mr.state = PSCParseState::Success;
        mr.result = remaining.mid(0, innerPos);
        pos += innerPos;
    }
    else {
        // 解析失败：根据 required 决定行为
        if (required) {
            collectError(remaining, errorType);
            mr.state = PSCParseState::Error;
        }
        else {
            mr.state = PSCParseState::Empty;  // 可选字段缺失，不是错误
        }
        // 注意：无论哪种失败，都不推进 pos（因为 innerPos == 0）
    }
    return mr;
}
void PSCTranslater::outputErr() const {
    if (m_errors.isEmpty()) {
        qDebug().noquote() << "noerror";
        return;
    }
    // 获取 PSCErrorType 的元对象枚举器
    const QMetaEnum metaEnum = QMetaEnum::fromType<PSCErrorType::errorType>();
    for (const PSCLangErr& err : m_errors) {
        // 将错误类型整数值转换为字符串键名
        const char* typeName = metaEnum.valueToKey(static_cast<int>(err.type));
        qDebug().noquote()
            << "Error:" << (typeName ? typeName : "UnknownError")
            << "at [" << err.begin << "-" << err.end << "]->"<<lastRaw.mid(err.begin,err.end-err.begin);
    }
}
// ---------- 成员函数 ----------
void PSCTranslater::collectError(const SubStr& where, PSCErrorType::errorType type) {
    m_errors.append({ where.absoluteBegin(), where.absoluteEnd(), type });
}

QString PSCTranslater::unescape(const QString& str) const {
    QString res;
    for (int i = 0; i < str.size(); ++i) {
        if (str[i] == QLatin1Char('\\') && i + 1 < str.size())
            res.append(str[++i]);
        else
            res.append(str[i]);
    }
    return res;
}

QVector<SubStr> PSCTranslater::splitArgs(const SubStr& argsStr) const {
    QVector<SubStr> parts;
    int segStart = 0;
    for (int i = 0; i < argsStr.size(); ++i) {
        QChar c = argsStr[i];
        if (c == QLatin1Char('\\')) {
            if (i + 1 < argsStr.size()) {
                QChar next = argsStr[i + 1];
                if (next == QLatin1Char(';') || next == QLatin1Char('\\'))
                    ++i;
            }
        }
        else if (c == QLatin1Char(';')) {
            parts.append(argsStr.mid(segStart, i - segStart));
            segStart = i + 1;
        }
    }
    parts.append(argsStr.mid(segStart));
    return parts;
}


MatchResult PSCTranslater::parseOp(const SubStr& str, int& pos) {
    return parseID(str, pos, PSCErrorType::InvalidOp, true);
}
MatchResult PSCTranslater::parseRecvName(const SubStr& str, int& pos) {
    return parseID(str, pos, PSCErrorType::InvalidRecv, true);
}
MatchResult PSCTranslater::parseUserDefID(const SubStr& str, int& pos) {
    return parseID(str, pos, PSCErrorType::InvalidRecv, false);
}

RecvResult PSCTranslater::parseHasRecv(const SubStr& str, int& pos) {
    RecvResult res;
    MatchResult nameMr = parseRecvName(str, pos);
    if (nameMr.state != PSCParseState::Success) {
        res.state = PSCParseState::Error;
        return res;
    }
    res.recvName = nameMr.result;

    while (pos < str.size() && str.at(pos).isSpace()) ++pos;
    if (pos < str.size() && str.at(pos) == QLatin1Char('(')) {
        ++pos;
        while (pos < str.size() && str.at(pos).isSpace()) ++pos;
        MatchResult udiMr = parseUserDefID(str, pos);
        res.userDefID = udiMr.state == PSCParseState::Success ? udiMr.result : SubStr();
        while (pos < str.size() && str.at(pos).isSpace()) ++pos;
        if (pos >= str.size() || str.at(pos) != QLatin1Char(')')) {
            SubStr errRegion = str.mid(pos, 1);
            collectError(errRegion.isEmpty() ? str : errRegion, PSCErrorType::MissingCloseParen);
            res.state = PSCParseState::Error;
            return res;
        }
        ++pos;
    }
    res.state = PSCParseState::Success;
    return res;
}

RecvResult PSCTranslater::parseRecv(const SubStr& str, int& pos) {
    while (pos < str.size() && str.at(pos).isSpace()) ++pos;
    if (pos >= str.size())
        return { PSCParseState::Empty, SubStr(), SubStr() };
    return parseHasRecv(str, pos);
}

LeftResult PSCTranslater::parseLeft(const SubStr& leftStr) {
    LeftResult left;
    int pos = 0;

    MatchResult opMr = parseOp(leftStr, pos);
    if (opMr.state != PSCParseState::Success) {
        left.state = PSCParseState::Error;
        return left;
    }
    left.op = opMr.result;

    RecvResult recv = parseRecv(leftStr, pos);
    if (recv.state == PSCParseState::Error) {
        left.state = PSCParseState::Error;
        return left;
    }
    left.recvName = recv.recvName;
    left.userDefID = recv.userDefID;

    while (pos < leftStr.size() && leftStr.at(pos).isSpace()) ++pos;
    if (pos != leftStr.size()) {
        SubStr trailing = leftStr.mid(pos);
        collectError(trailing, PSCErrorType::TrailingChars);
        left.state = PSCParseState::Error;
        return left;
    }

    left.state = PSCParseState::Success;
    return left;
}

ArgResult PSCTranslater::parseCall(const SubStr& argStr) {
    ArgResult res;
    if (!argStr.startsWith(QLatin1Char('@'))) {
        res.state = PSCParseState::Error;
        return res;
    }
    int pos = 1;
    MatchResult recvMr = parseRecvName(argStr, pos);
    if (recvMr.state != PSCParseState::Success) {
        res.state = PSCParseState::Error;
        return res;
    }
    if (pos >= argStr.size() || argStr.at(pos) != QLatin1Char('(')) {
        SubStr errRegion = (pos < argStr.size()) ? argStr.mid(pos, 1) : argStr;
        collectError(errRegion, PSCErrorType::InvalidCall);
        res.state = PSCParseState::Error;
        return res;
    }
    ++pos;
    MatchResult udiMr = parseUserDefID(argStr, pos);
    if (pos >= argStr.size() || argStr.at(pos) != QLatin1Char(')')) {
        SubStr errRegion = (pos < argStr.size()) ? argStr.mid(pos, 1) : argStr;
        collectError(errRegion, PSCErrorType::MissingCloseParen);
        res.state = PSCParseState::Error;
        return res;
    }
    ++pos;
    while (pos < argStr.size() && argStr.at(pos).isSpace()) ++pos;
    if (pos != argStr.size()) {
        SubStr trailing = argStr.mid(pos);
        collectError(trailing, PSCErrorType::TrailingChars);
        res.state = PSCParseState::Error;
        return res;
    }

    res.state = PSCParseState::Success;
    res.arg.valid = true;
    res.arg.type = PSCCommandArg::call;
    res.arg.body = recvMr.result.toString();
    res.arg.argForCall = udiMr.state == PSCParseState::Success ? udiMr.result.toString() : QString();
    return res;
}

ArgResult PSCTranslater::parseRawStr(const SubStr& str) {
    ArgResult res;
    res.state = PSCParseState::Success;
    res.arg.valid = true;
    res.arg.type = PSCCommandArg::rawStr;
    res.arg.body = unescape(str.toString());
    return res;
}

ArgResult PSCTranslater::parseArg(const SubStr& argStr) {
    if (argStr.startsWith(QLatin1Char('@'))) {
        // 以 @ 开头必须按 call 解析，解析失败则直接报错，不降级为 rawStr
        return parseCall(argStr);
    }
    else {
        return parseRawStr(argStr);
    }
}

ArgsResult PSCTranslater::parseArgs(const SubStr& argsStr) {
    ArgsResult argsRes;
    QVector<SubStr> parts = splitArgs(argsStr);
    for (const SubStr& part : parts) {
        ArgResult arg = parseArg(part);
        argsRes.args.append(arg.arg);
    }
    argsRes.state = PSCParseState::Success; // 参数解析整体成功
    return argsRes;
}

PSCCommand PSCTranslater::translate(const QString& raw) {
    m_errors.clear();  // 每次翻译前清空错误
    lastRaw = raw;

    PSCCommand cmd;
    QString trimmed = raw.trimmed();
    SubStr full(trimmed, 0, trimmed.size());

    if (!full.startsWith(QLatin1Char('@'))) {
        collectError(full, PSCErrorType::InvalidOp);
        return cmd;
    }
    SubStr afterAt = full.mid(1);

    // 寻找冒号
    int colonRel = afterAt.indexOf(QLatin1Char(':'));

    SubStr leftStr;
    SubStr argsStr;      // 实际使用与否取决于是否有冒号，这里声明但不一定赋值
    if (colonRel == -1) {
        // 没有冒号：整个 afterAt 视为左侧（操作符+接收器部分）
        leftStr = afterAt.trimmed();
        // argsStr 保持未赋值状态，之后不会用它
    }
    else {
        // 有冒号：分割左侧和参数部分
        leftStr = afterAt.mid(0, colonRel).trimmed();
        argsStr = afterAt.mid(colonRel + 1);
    }

    LeftResult left = parseLeft(leftStr);
    ArgsResult args;
    if (colonRel != -1) {
        args = parseArgs(argsStr);   // 有冒号才解析参数
    }
    else {
        // 没有冒号时参数列表为空，args 默认构造即可
    }

    // 组装最终命令（无论 left 是否成功都填充字段）
    cmd.valid = (left.state == PSCParseState::Success);
    cmd.op = left.op.toString();
    cmd.recv = left.recvName.toString();
    cmd.recvUDI = left.userDefID.toString();
    cmd.args = args.args;

    return cmd;
}