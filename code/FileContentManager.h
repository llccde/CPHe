#pragma once
#include <QMap>
#include <QVector>
#include <QString>
#include <algorithm>
#include "CodeSnippet.h"   // 定义了 CodeRnage

using FileRange = CodeRnage;

class FileContentManager {
public:
    void accept(const FileRange& range);

    void removeAccept(const FileRange& range);

    QVector<QString> readFile(const QString& filePath, const QVector<QString>& originContent);

    QMap<QString, QVector<QString>> readFiles(const QMap<QString, QVector<QString>>& originContents);


private:
    struct Operation {
        bool isAccept;      // true=接受，false=拒绝
        FileRange range;
    };

    // 通用实现：给定文件路径和内容，应用 m_operations 中匹配该文件的项
    QVector<QString> applyOperationsForFile(const QString& filePath, const QVector<QString>& originContent) const;

    QVector<Operation> m_operations;  // 按调用顺序记录的操作序列
};