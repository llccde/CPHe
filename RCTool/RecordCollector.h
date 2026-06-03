#pragma once
#include"../AIWork/Interpreter.h"
#include<qfile.h>
#include<qfileinfo.h>
#include<qdir.h>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>

using FilePath = QString;
class RecordCollector {

public:
	QString workingFolder;
};

using FilePath = QString;

class BaseRefTool {
public:
    QString workingFolder;

    // 获取工作目录下所有路径后缀与 endWith 完全匹配的文件
    // endWith 是一个路径组件列表，例如 {"b", "ec.txt"} 表示匹配 ".../b/ec.txt"
    QVector<FilePath> getFileByPathEnd(const QVector<QString>& endWith) {
        QVector<FilePath> result;
        if (endWith.isEmpty())
            return result;

        // 遍历工作目录下所有文件
        QDirIterator it(workingFolder, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QString absPath = it.fileInfo().absoluteFilePath();
            // 获取相对于工作目录的路径，统一使用 '/' 分割
            QString relPath = QDir(workingFolder).relativeFilePath(absPath);
            // 将分隔符统一为 '/'
            relPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
            const QStringList parts = relPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);

            // 检查后缀是否匹配
            if (parts.size() < endWith.size())
                continue;
            bool match = true;
            for (int i = 0; i < endWith.size(); ++i) {
                if (parts[parts.size() - endWith.size() + i] != endWith[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                result.append(absPath);
            }
        }
        return result;
    }
};

class BaseRefFeature {
public:
    QString workingFolder;

    // 解析代码文件中的 #include 指令，提取对应的文件路径
    QVector<FilePath> refIncludeFiles(QFile& codeFile) {
        QVector<FilePath> result;
        if (!codeFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return result;

        QTextStream in(&codeFile);
        const QString content = in.readAll();
        codeFile.close();

        // 整行必须以 #include 开头（前面仅允许空白）
        QRegularExpression includeRegex(R"((?m)^[ \t]*#include\s*[<"]([^">]+)[">])");
        QRegularExpressionMatchIterator it = includeRegex.globalMatch(content);

        BaseRefTool tool;
        tool.workingFolder = workingFolder;

        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString includePath = match.captured(1).trimmed();

            // 清理路径...
            QString cleaned = QDir::cleanPath(includePath);
            while (cleaned.startsWith(QLatin1String("../")) ||
                cleaned.startsWith(QLatin1String("..\\"))) {
                cleaned = cleaned.mid(3);
            }
            if (cleaned.startsWith(QLatin1String("./")) ||
                cleaned.startsWith(QLatin1String(".\\"))) {
                cleaned = cleaned.mid(2);
            }

            if (cleaned.isEmpty())
                continue;

            cleaned.replace(QLatin1Char('\\'), QLatin1Char('/'));
            const QStringList suffixParts = cleaned.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            if (suffixParts.isEmpty())
                continue;

            QVector<FilePath> candidates = tool.getFileByPathEnd(
                QVector<QString>(suffixParts.begin(), suffixParts.end()));

            if (!candidates.isEmpty()) {
                FilePath best = candidates.first();
                int bestDepth = QDir(workingFolder).relativeFilePath(best)
                    .count(QLatin1Char('/'));
                for (int i = 1; i < candidates.size(); ++i) {
                    int depth = QDir(workingFolder).relativeFilePath(candidates[i])
                        .count(QLatin1Char('/'));
                    if (depth < bestDepth) {
                        best = candidates[i];
                        bestDepth = depth;
                    }
                }
                result.append(best);
            }
        }
        return result;
    }
};