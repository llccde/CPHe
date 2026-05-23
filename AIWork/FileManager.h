#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace awf {

    class LineBaseFileManager {
    public:
        struct Record {
            QString content;   // 行内容，可扩展其他字段
        };

        /// 从文件构造，后续可用 writeBack() 写回原文件
        explicit LineBaseFileManager(const QString& path);

        /// 从字符串列表构造（每行一个元素），不关联任何文件
        explicit LineBaseFileManager(const QStringList& content);

        /// 写回构造时提供的文件（仅当通过文件路径构造时有效）
        bool writeBack();

        /// 将当前内容写入指定文件
        bool writeTo(const QString& path);

        /// 在原始文件第 line 行（0-based）之后插入一行
        void insertAfterLineOfOrigin(int line, const QString& content);

        /// 在原始文件第 line 行（0-based）之前插入一行
        void insertBeforeLineOfOrigin(int line, const QString& content);

        /// 获取拼接后的完整内容（原始行 + 插入行）
        QStringList getContent() const;

        /// 返回原始文件的行数
        int originalLineCount() const;

    private:
        QStringList m_originalLines;                // 原始行内容
        QString m_filePath;                         // 关联的文件路径（可为空）
        QVector<QVector<Record>> m_records;         // 插入桶，大小 = 原始行数 + 1

        void initRecords();
    public:

        LineBaseFileManager() = default;
    };

} // namespace awf