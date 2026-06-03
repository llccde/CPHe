#include"ClangTool.h"
#include <clang-c/Index.h>
#include <QFile>
#include <QTextStream>
#include <QDebug>
using namespace awf;
QVector<LineInfo> ClangTool::analyzeFile(const QString& filePath) {
    QVector<LineInfo> result;

    // 1. 读取源文件全部行，初始化 LineInfo 结构
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file:" << filePath;
        return result;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        LineInfo info;
        info.rawLine = in.readLine() + "\n";
        result.append(info);
    }
    file.close();

    if (result.isEmpty()) {
        return result;
    }

    // 2. 使用 libclang 解析，保留注释 token
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        filePath.toStdString().c_str(),
        nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_DetailedPreprocessingRecord // 关键：保留所有注释
    );

    if (!unit) {
        clang_disposeIndex(index);
        qWarning() << "Failed to parse translation unit:" << filePath;
        return result;
    }

    // 3. 获取整个文件的 token 序列
    CXSourceRange range = clang_getCursorExtent(clang_getTranslationUnitCursor(unit));
    CXToken* tokens = nullptr;
    unsigned numTokens = 0;
    clang_tokenize(unit, range, &tokens, &numTokens);

    // 记录每一行是否存在非注释 token
    std::vector<bool> hasCode(result.size(), false);

    // 4. 遍历 token，分类处理
    for (unsigned i = 0; i < numTokens; ++i) {
        CXTokenKind kind = clang_getTokenKind(tokens[i]);
        CXSourceLocation loc = clang_getTokenLocation(unit, tokens[i]);
        unsigned line, column;
        clang_getFileLocation(loc, nullptr, &line, &column, nullptr);

        if (line == 0 || line > result.size()) {
            continue; // 忽略无效行号
        }

        if (kind == CXToken_Comment) {
            // ── 注释 token：获取原始文本并确定范围 ──
            CXString spelling = clang_getTokenSpelling(unit, tokens[i]);
            QString commentRaw = QString::fromUtf8(clang_getCString(spelling));
            clang_disposeString(spelling);

            CXSourceRange tokenRange = clang_getTokenExtent(unit, tokens[i]);
            CXSourceLocation endLoc = clang_getRangeEnd(tokenRange);
            unsigned endLine;
            clang_getFileLocation(endLoc, nullptr, &endLine, nullptr, nullptr);
            if (endLine == 0) endLine = line; // 安全回退

            // 判断是否为跨行注释（起始行 != 结束行）
            bool isMultiLine = (endLine != line);

            if (isMultiLine) {
                // 标记起始行和结束行的多行注释标志
                if (line <= result.size()) {
                    result[line - 1].hasMutiLineCommentBegin = true;
                }
                if (endLine <= result.size()) {
                    result[endLine - 1].hasMutiLineCommentEnd = true;
                }
            }

            // ── 按行拆分注释文本，并去除注释标记 ──
            // 注释可能包含换行，按 \n 分割
            QStringList commentLines = commentRaw.split('\n');
            // 如果 token 覆盖多行，commentLines 的数量应当与行数匹配
            // 但考虑到行内可能还有其他注释，我们将每行内容去掉注释符号后追加到对应行
            for (int offset = 0; offset < commentLines.size(); ++offset) {
                unsigned targetLine = line + offset;
                if (targetLine < 1 || targetLine > result.size()) break;

                QString lineText = commentLines[offset];

                // 去除行注释或块注释的定界符
                // 首行可能以 /* 或 // 开头
                if (offset == 0) {
                    if (lineText.startsWith("//")) {
                        lineText = lineText.mid(2);
                    }
                    else if (lineText.startsWith("/*")) {
                        lineText = lineText.mid(2);
                        // 如果单行就闭合了 */，去除尾部的 */
                        if (lineText.endsWith("*/")) {
                            lineText.chop(2);
                        }
                    }
                }
                // 中间行：可能以 * 开头，也可能是普通内容
                // 最后一行的末尾可能以 */ 结束
                if (offset == commentLines.size() - 1 && lineText.endsWith("*/")) {
                    lineText.chop(2);
                }

                // 清理首尾空白
                lineText = lineText.trimmed();
                if (lineText.isEmpty()) continue;

                // 追加到该行的 commentText 中（用空格分隔不同注释 token）
                if (!result[targetLine - 1].commentText.isEmpty()) {
                    result[targetLine - 1].commentText += " ";
                }
                result[targetLine - 1].commentText += lineText;
            }
        }
        else {
            // 非注释 token：标记该行包含代码
            hasCode[line - 1] = true;
        }
    }

    // 5. 确定每行的最终注释状态
    for (int i = 0; i < result.size(); ++i) {
        // 只有不含任何代码 token 且注释内容非空的行才视为纯注释行
        result[i].isCommentOnly = (!hasCode[i] && !result[i].commentText.isEmpty());
    }

    // 6. 清理资源
    clang_disposeTokens(unit, tokens, numTokens);
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return result;
}

QString awf::ClangTool::getAllTheFile(const QString& p)
{
    QFile f(p);
    if (f.exists())
    {
        QString s(f.readAll());return s;
    }
    else
    {
        return"file not found";
    }
}

void awf::ClangTool::FormatTab(QVector<QString>& code, const QString refObj)
{
    if (code.isEmpty())
        return;

    // 计算一行的前导空白所等价的空格数（1 tab = 4 空格）
    auto calcIndentSpaces = [](const QString& line) -> int {
        int spaces = 0;
        for (const QChar& ch : line) {
            if (ch == QLatin1Char(' '))
                ++spaces;
            else if (ch == QLatin1Char('\t'))
                spaces += 4;
            else
                break;
        }
        return spaces;
        };

    // 提取一行的前导空白字符串
    auto getIndentStr = [](const QString& line) -> QString {
        int i = 0;
        while (i < line.size() && (line[i] == QLatin1Char(' ') || line[i] == QLatin1Char('\t')))
            ++i;
        return line.left(i);
        };

    // 1. 记录每行的缩进空格数，并收集所有不同的缩进长度
    QVector<int> lineIndents(code.size());
    QSet<int> indentSet;
    for (int i = 0; i < code.size(); ++i) {
        int sp = calcIndentSpaces(code[i]);
        lineIndents[i] = sp;
        indentSet.insert(sp);
    }

    // 2. 将所有不同缩进长度排序
    QList<int> sortedIndents = indentSet.values();
    std::sort(sortedIndents.begin(), sortedIndents.end());

    // 3. code[0] 的缩进等级为 0，找到它在排序列表中的位置
    int firstIndent = lineIndents[0];
    int firstIdx = sortedIndents.indexOf(firstIndent);

    // 4. 建立 原缩进长度 -> 缩进等级 的映射
    QMap<int, int> levelMap;
    for (int i = 0; i < sortedIndents.size(); ++i) {
        levelMap.insert(sortedIndents[i], i - firstIdx); // 等级可能为负
    }

    // 5. 解析 refObj 的缩进
    int refSpaces = calcIndentSpaces(refObj);
    QString refIndentStr = getIndentStr(refObj);
    // 若 ref 的缩进中没有空格，则使用 tab；否则使用空格
    bool useTabs = !refIndentStr.contains(QLatin1Char(' '));

    // 6. 按等级重新设置每一行的缩进
    for (int i = 0; i < code.size(); ++i) {
        int level = levelMap.value(lineIndents[i]);
        // 新缩进空格数 = 等级 * 4 + code[0] 的基准缩进（即 ref 的缩进）
        int newSpaces = level * 4 + refSpaces;
        if (newSpaces < 0)
            newSpaces = 0; // 缩进符最小为 0 个

        QString newIndent;
        if (useTabs) {
            int tabs = newSpaces / 4; // 此时 newSpaces 一定是 4 的倍数
            newIndent = QString(tabs, QLatin1Char('\t'));
        }
        else {
            newIndent = QString(newSpaces, QLatin1Char(' '));
        }

        // 去掉该行原有的前导空白，拼接上新缩进和剩余内容
        const QString& oldLine = code[i];
        int contentStart = 0;
        while (contentStart < oldLine.size() &&
            (oldLine[contentStart] == QLatin1Char(' ') || oldLine[contentStart] == QLatin1Char('\t')))
            ++contentStart;
        code[i] = newIndent + oldLine.mid(contentStart);
    }
}

QString awf::ClangTool::clearSingleLineBreak(const QString& code)
{
    auto line = code;
    int i = 0;
    const int len = line.length();
    while (i < len && line[len - 1 - i] == QLatin1Char('\n')) {
        ++i;
    }
    line.chop(i);
    return line;
}

void awf::ClangTool::clearLineBreak(QVector<QString>& code)
{
    for (auto& line : code) {
        int i = 0;
        const int len = line.length();
        while (i < len && line[len - 1 - i] == QLatin1Char('\n')) {
            ++i;
        }
        line.chop(i);
    }
}

QString awf::ClangTool::SameTab(const QString& ori, const QString& ref)
{
    // 提取 ref 开头连续的空格或制表符，作为“缩进序列”
    QString indent;
    for (int i = 0; i < ref.size(); ++i) {
        const QChar ch = ref.at(i);
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))
            indent.append(ch);
        else
            break;
    }

    // 如果 ref 没有缩进，则无需插入，直接返回原字符串
    if (indent.isEmpty())
        return ori;

    QString result;
    // 预分配内存，提高效率
    result.reserve(ori.size() + indent.size() * (ori.count(QLatin1Char('\n')) + 1));

    // 在字符串开头插入缩进序列
    result.append(indent);

    // 遍历原字符串，在每个“中间出现”的换行符之后插入缩进序列
    // （即不在末尾的换行符才处理，避免在最后一行的空行后添加多余缩进）
    for (int i = 0; i < ori.size(); ++i) {
        const QChar ch = ori.at(i);
        result.append(ch);
        if (ch == QLatin1Char('\n') && i != ori.size() - 1) {
            result.append(indent);
        }
    }

    return result;
}
#include<functional>
void func(std::function<void()> a = []() {
    }) 
{
}
struct GetSymbolDefVisitorData {
    const QString& symbolName;          // 要查找的符号名称
    const QByteArray& utf8Code;         // 原始代码的UTF-8字节数组
    QVector<QString>& results;          // 存储提取的定义文本
    bool hadError;                      // 是否发生错误
    ExceptionCollector& ec;
};

// 静态遍历回调函数

static CXChildVisitResult getSymbolDefVisitCursor(CXCursor cursor, CXCursor parent, CXClientData clientData) {
    GetSymbolDefVisitorData* data = static_cast<GetSymbolDefVisitorData*>(clientData);
    if (data->hadError)
        return CXChildVisit_Break;

    // 只处理声明（包括定义）
    CXCursorKind kind = clang_getCursorKind(cursor);
    if (!clang_isDeclaration(kind)) {
        return CXChildVisit_Continue;
    }

    // 获取cursor的拼写名称
    CXString nameSpelling = clang_getCursorSpelling(cursor);
    QString curName = QString::fromUtf8(clang_getCString(nameSpelling));
    clang_disposeString(nameSpelling);

    // 检查名称是否匹配（区分大小写，完全一致）
    if (curName != data->symbolName) {
        return CXChildVisit_Continue;
    }

    // 检查是否为定义（而非仅仅声明）
    if (!clang_isCursorDefinition(cursor)) {
        return CXChildVisit_Continue;
    }

    // 提取定义的范围文本
    CXSourceRange range = clang_getCursorExtent(cursor);
    if (clang_Range_isNull(range)) {
        // 空范围，跳过
        return CXChildVisit_Continue;
    }

    CXSourceLocation startLoc = clang_getRangeStart(range);
    CXSourceLocation endLoc = clang_getRangeEnd(range);

    unsigned startOffset = 0, endOffset = 0;
    CXFile startFile = nullptr, endFile = nullptr;
    clang_getFileLocation(startLoc, &startFile, nullptr, nullptr, &startOffset);
    clang_getFileLocation(endLoc, &endFile, nullptr, nullptr, &endOffset);

    if (!startFile || !endFile || startOffset > endOffset) {
        // 无效偏移量，跳过
        return CXChildVisit_Continue;
    }

    // 从原始UTF-8代码中截取子串
    int length = static_cast<int>(endOffset - startOffset);
    if (length <= 0) {
        return CXChildVisit_Continue;
    }

    QByteArray definitionBytes = data->utf8Code.mid(static_cast<int>(startOffset), length);
    QString definitionText = QString::fromUtf8(definitionBytes);
    if (!definitionText.isEmpty()) {
        data->results.append(definitionText);
    }

    return CXChildVisit_Continue;
}

QVector<QString> ClangTool::getSymbolDef(const QString& code, const QString& symbolName,const QString& fileName ) {
    QVector<QString> results;
    QByteArray utf8Code = code.toUtf8();

    // 初始化 libclang 索引
    CXIndex index = clang_createIndex(1, 1);
    if (!index) {
        ec.Err(QString("Failed to create libclang index"));
        return results;
    }
    auto byte = fileName.toUtf8();
    // 准备未保存的文件（内存代码）
    const char* filename2 = "input.cpp";
    CXUnsavedFile unsavedFile = {
        filename2,
        utf8Code.constData(),
        static_cast<unsigned long>(utf8Code.size())
    };

    // 命令行参数（可以添加更多需要的编译标志）
    const char* args[] = { "-std=c++17","-x","c++" };  // 显式指定语言为C++
    int numArgs = sizeof(args) / sizeof(args[0]);

    // 解析翻译单元
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index, filename2, args, numArgs,
        &unsavedFile, 1,
        CXTranslationUnit_None |CXTranslationUnit_Incomplete
    );

    if (!tu) {
        clang_disposeIndex(index);
        ec.Err(QString("Failed to parse translation unit"));
        return results;
    }

    // ---------- 新增：诊断回调处理 ----------
    bool hasDiagError = false;
    unsigned numDiags = clang_getNumDiagnostics(tu);
    for (unsigned i = 0; i < numDiags; ++i) {
        CXDiagnostic diag = clang_getDiagnostic(tu, i);
        CXString diagStr = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
        QString msg = QString::fromUtf8(clang_getCString(diagStr));

        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
        if (severity >= CXDiagnostic_Error) {
            ec.Err(msg);
            
        }
        else if (severity >= CXDiagnostic_Warning) {
            ec.Wrn(msg);
        }

        clang_disposeString(diagStr);
        clang_disposeDiagnostic(diag);
    }
    // ----------------------------------------

    // 获取根游标
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

    // 准备遍历数据（hadError 初始值考虑诊断结果）
    GetSymbolDefVisitorData visitorData = {
        symbolName,
        utf8Code,
        results,
        hasDiagError,  // 初始错误状态来自诊断
        ec
    };

    // 遍历AST
    clang_visitChildren(rootCursor, getSymbolDefVisitCursor, &visitorData);

    if (visitorData.hadError) {
        ec.Wrn(QString("Errors occurred while traversing AST, partial results may be returned"));
    }

    // 清理资源
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return results;
}