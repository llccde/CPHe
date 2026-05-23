// interpreter.cpp
#include"Interpreter.h"
#include"ClangTool.h"
#include <QRegularExpression>
#include"CommentTool.h"
using namespace awf;
void Interpreter::loadFile(QString filePath) {
    CommentTool tool;
    mLines = tool.analyzeFile(filePath,"//","/*","*/");
    mCommandCache.clear();
}

int Interpreter::rowCount() {
    return mLines.size();
}

QString Interpreter::getSource(int row) {
    if (row < 0 || row >= mLines.size()) return QString();
    return mLines[row].rawLine;
}

bool Interpreter::isCommandComment(int row) {
    if (row < 0 || row >= mLines.size()) return false;
    if (!mLines[row].isCommentOnly) return false;
    // 注释内容去掉前导空白后是否以 '@' 开头
    QString text = mLines[row].commentText.trimmed();
    return text.startsWith('@');
}

M_Command Interpreter::getCommandOf(int row) {
    if (mCommandCache.contains(row)) {
        return mCommandCache[row];
    }

    M_Command cmd;
    if (row >= 0 && row < mLines.size()) {
        cmd.hasMutiLineCommentBegin = mLines[row].hasMutiLineCommentBegin;
        cmd.hasMutiLineCommentEnd = mLines[row].hasMutiLineCommentEnd;
    }
    else {
        cmd.hasMutiLineCommentBegin = false;
        cmd.hasMutiLineCommentEnd = false;
    }


    cmd.type = M_OperatorType::notCommand;

    if (row < 0 || row >= mLines.size()) {
        mCommandCache[row] = cmd;
        return cmd;
    }

    const LineInfo& line = mLines[row];

    if (!line.isCommentOnly) {
        cmd.type = M_OperatorType::notCommand;
        cmd.arg = line.rawLine;
        mCommandCache[row] = cmd;
        return cmd;
    }

    // 注释行
    QString text = line.commentText.trimmed();
    if (text.startsWith('*')) {
        for (auto c:text)
        {
            if (c == ' ' || c == '*') {
                continue;
            }
            else if(c=='@'){
                riseWarning("comment line start with '*',and has '@' at right,if it should be a command comment,please remove any '*' before'@' ");
            }
        }
       
    }
    if (!text.startsWith('@')) {
        // 普通注释
        cmd.type = M_OperatorType::normalComment;
        mCommandCache[row] = cmd;
        return cmd;
    }

    text = text.mid(1).trimmed(); // 先 trim，避免 "@}  " 不匹配

    // ★ 特殊处理 @} 结束标记
    if (text == "}") {
        cmd.type = MP::end;
        mCommandCache[row] = cmd;
        return cmd;
    }



    QRegularExpression re("^([A-Za-z_][A-Za-z0-9_]*)(.*)$");
    QRegularExpressionMatch match = re.match(text);
    if (!match.hasMatch()) {
        // 不合法格式，视为普通注释（但 @} 已经在上面处理了）
        cmd.type = M_OperatorType::normalComment;
        cmd.arg = line.commentText;
        mCommandCache[row] = cmd;
        return cmd;
    }

    QString opName = match.captured(1);
    QString rest = match.captured(2).trimmed();

    // 将操作符字符串转为枚举
    cmd.type = stringToOperatorType(opName);
    if (cmd.type == MP::notCommand) {
        cmd.arg = line.rawLine;
        ec.riseWrn("无效的指令标记 \"" + opName+"\"");
    }
    // 处理长标记 @xxx{
    if (rest.startsWith('{')) {
        cmd.isLongOperator = true;
        // 去掉开头的 '{'，后面可能还有参数（如 @fill{ 后面直接跟换行）
        // 当前设计中长标记的 '{' 后一般换行，不直接跟参数；但可兼容
        rest = rest.mid(1).trimmed();
        // 如果后面还有内容，当作 arg 的第一行
        if (!rest.isEmpty()) {
            cmd.arg = rest;
        }
    }
    // 处理参数：@fill,arg1=val1,arg2=val2 或 @fill arg 或 @fill: arg
    else if (rest.startsWith(',')) {
        // 参数形式
        rest = rest.mid(1); // 跳过逗号
        parseArguments(rest, cmd.args);
        // 如果有剩余未解析为 key=value 的纯文本，存入 arg
        // 此处简单处理：如果不是 key=value 对，整段作为 arg
        // 更健壮的方式：找到第一个空格前的部分为 arg，但需结合语法设计
        // 假设当前设计： @fill,arg1=xx,arg2=xx 没有独立 arg
    }
    else if (rest.startsWith(':')) {
        // @fill: 描述
        cmd.arg = rest.mid(1).trimmed();
    }
    else {
        // @fill 描述 （空格分隔）
        cmd.arg = rest;
    }

    // 如果没有 arg 且不是长标记，可能 args 中有原始描述，进一步处理
    // 简单起见，如果 cmd.arg 为空且不是长标记，则将整个 rest 作为 arg
    if (cmd.arg.isEmpty() && !cmd.isLongOperator && cmd.args.isEmpty()) {
        cmd.arg = rest;
    }



    mCommandCache[row] = cmd;




    return cmd;
}


// 解析 "key1=val1,key2=val2" 形式的参数
void Interpreter::parseArguments(const QString& argPart, QMap<QString, QString>& args) {
    const QStringList pairs = argPart.split(',', Qt::SkipEmptyParts);
    for (const QString& pair : pairs) {
        QString p = pair.trimmed();
        int eqIdx = p.indexOf('=');
        if (eqIdx > 0) {
            QString key = p.left(eqIdx).trimmed();
            QString val = p.mid(eqIdx + 1).trimmed();
            args[key] = val;
        }
        else {
            // 不符合 key=value 的部分，忽略或合并到 arg 中（由上层处理）
        }
    }
}
