#include "CppCodeFileReader.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

// 引入 clang-format 相关头文件
#include <clang/Format/Format.h>
#include <clang/Tooling/Inclusions/IncludeStyle.h>
#include <llvm/Support/raw_ostream.h>
#include "clang/Tooling/Core/Replacement.h"
#include "llvm/ADT/StringRef.h"
CppCodeFileReader::CppCodeFileReader(bool formatIndent, CodePosition pos, int indentLength)
    : formatIndent(formatIndent), indentLength(indentLength),pos(pos)
{
    readLineRawIntoBuffer();
    if (formatIndent) {
        handleIndent();
    }
}

QString CppCodeFileReader::readLine(int offset)
{
    if (offset >= 0 && offset < buffer.size()) {
        return buffer[offset];
    }
    return QString();
}

int CppCodeFileReader::getRowCount()
{
    return buffer.size();
}

void CppCodeFileReader::readLineRawIntoBuffer()
{
    QFile file(pos.file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return; // 文件打开失败，buffer为空
    }

    QTextStream stream(&file);
    int currentLine = 1;
    QString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        if (currentLine >= pos.rowBegin && currentLine <= pos.rowEnd) {
            // 根据行列范围截取当前行
            if (currentLine == pos.rowBegin && currentLine == pos.rowEnd) {
                // 只有一行的情况，同时考虑起始列和结束列
                int start = pos.columBegin - 1;
                int end = pos.columnEnd - 1;
                if (start < 0) start = 0;
                if (end >= line.length()) end = line.length() - 1;
                if (start <= end) {
                    line = line.mid(start, end - start + 1);
                }
                else {
                    line.clear();
                }
            }
            else if (currentLine == pos.rowBegin) {
                // 起始行，从起始列开始
                int start = pos.columBegin - 1;
                if (start < 0) start = 0;
                line = line.mid(start);
            }
            else if (currentLine == pos.rowEnd) {
                // 结束行，到结束列结束
                int end = pos.columnEnd - 1;
                if (end < 0) end = -1;
                line = line.left(end + 1);
            }
            // 中间行保持原样
            buffer.push_back(line);
        }
        ++currentLine;
        if (currentLine > pos.rowEnd) break;
    }
    file.close();
}

void CppCodeFileReader::handleIndent()
{
    // 将 buffer 中的所有行合并为一个字符串，用换行符分隔
    std::string codeStr;
    for (const auto& line : buffer) {
        codeStr += line.toStdString() + "\n";
    }

    // 配置 clang-format 风格
    clang::format::FormatStyle style = clang::format::getLLVMStyle();
    // 根据 indentString 设置缩进宽度（假设 indentString 由空格组成）
    // 如果 indentString 包含制表符，则设置 UseTab = UT_Always
    style.IndentWidth = indentLength;
    //if (indentString.contains('\t')) {
    //    style.UseTab = clang::format::FormatStyle::UT_Always;
    //    // 如果同时包含空格和制表符，可能还需进一步处理，这里简化
    //}
    // 其他常用选项（可根据需要调整）
    style.ColumnLimit = 0;       // 不限制行宽
    style.BreakBeforeBraces = clang::format::FormatStyle::BS_Allman; // 大括号换行风格（可自定义）
    // 可在此处添加更多风格设置，例如基于 indentString 设定 TabWidth 等

    
    auto replacement = clang::format::reformat(
        style,
        codeStr,
        clang::tooling::Range(0, codeStr.length()),
        ".cpp" 
    );
    llvm::Expected<std::string> formatted = clang::tooling::applyAllReplacements(codeStr, replacement);

    if (formatted) {
        // 将格式化后的字符串按行分割回 buffer
        QString formattedQString = QString::fromStdString(*formatted);
        buffer.clear();
        // 按行分割，忽略末尾空行（若有）
        QStringList lines = formattedQString.split("\n", Qt::KeepEmptyParts);
        for (const auto& line : lines) {
            buffer.push_back(line);
        }
        // 如果最后一行是空行且原文件最后有换行，保持原样；这里简单移除末尾空行
        while (!buffer.empty() && buffer[buffer.size()-1].isEmpty()) {
            buffer.pop_back();
        }
    }
    else {
        // 格式化失败，保留原始内容（不处理）
        llvm::handleAllErrors(formatted.takeError(), [](const llvm::ErrorInfoBase& E) {
            // 可以添加日志输出
            qDebug() << "Clang-format error:" << E.message().c_str();
            });
    }
}