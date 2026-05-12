#pragma once
#include<qstring.h>
#include"qfile.h"
#include"PSCContext.h"
#include"RuntimeErrcollector.h"
static auto codeAnippetDref = [](PSCContext* ctx, QString id) -> PSCVar {
    if (!ctx || !ctx->collector) {  // 增加 collector 判空
        // 无法上报错误，只能静默返回
        return {};
    }

    // 1. 获取代码段信息
    if (!ctx->cppContext.savedSnippet.contains(id)) {
        ctx->collector->putError(QString("codeAnippetDref: snippet not found, id = %1").arg(id));
        return {};
    }
    const CppCodeAnalyzerResult::CodeSnippet& snippet = ctx->cppContext.savedSnippet[id];

    // 2. 获取文件路径（优先使用 savedSnippetFilePath）
    QString filePath;
    if (ctx->cppContext.savedSnippetFilePath.contains(id)) {
        filePath = ctx->cppContext.savedSnippetFilePath[id];
    }
    else {
        ctx->collector->putError(QString("codeAnippetDref: can't determine file path for snippet id %1").arg(id));
        return {};
    }

    // 3. 打开文件并读取所有行
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ctx->collector->putError(QString("codeAnippetDref: failed to open file %1").arg(filePath));
        return {};
    }
    QTextStream stream(&file);
    QStringList lines;
    while (!stream.atEnd()) {
        lines.append(stream.readLine());
    }
    file.close();

    // 4. 验证行范围有效性（行号从 1 开始）
    long long beginLine = snippet.beginLine;
    long long endLine = snippet.endLine;
    if (beginLine < 1 || endLine > lines.size() || beginLine > endLine) {
        ctx->collector->putError(QString("codeAnippetDref: invalid line range %1-%2, file has %3 lines")
            .arg(beginLine).arg(endLine).arg(lines.size()));
        return {};
    }

    // 5. 提取文本，处理列裁剪（列号从 1 开始，区间 [beginColumn, endColumn)）
    QString result;
    for (long long lineNo = beginLine; lineNo <= endLine; ++lineNo) {
        QString lineText = lines[lineNo - 1];
        long long startCol = (lineNo == beginLine) ? snippet.beginColumn : 1;
        long long endCol = (lineNo == endLine) ? snippet.endColumn : lineText.length() + 1;

        // 调整边界
        if (startCol < 1) startCol = 1;
        if (endCol > lineText.length() + 1) endCol = lineText.length() + 1;
        if (startCol > endCol) startCol = endCol;

        if (startCol <= lineText.length()) {
            QString part = lineText.mid(startCol - 1, endCol - startCol);
            if (lineNo > beginLine) result += '\n';
            result += part;
        }
    }

    return PSCVar(result);
    };