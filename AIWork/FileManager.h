#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QSet>   // 新增，用于记录待移除行
#include<vector>
namespace awf {

    class LineBaseFileManager {
    public:
        struct Record {
            QString content;
        };
        struct RmRecord {
            int from,to;
        };

        explicit LineBaseFileManager(const QString& path);
        explicit LineBaseFileManager(const QStringList& content);

        bool writeBack();
        bool writeTo(const QString& path);
        QString& getOrigin(int row);
        void insertAfterLineOfOrigin(int line, const QString& content);
        void insertBeforeLineOfOrigin(int line, const QString& content);

        QStringList getContent() const;

        int originalLineCount() const;

        // 【新增】标记移除 [from, to] 的原始行，在 getContent() 时生效
        void removeFromTo(int from, int to);

    private:
        QStringList m_originalLines;
        QString m_filePath;
        QVector<QVector<Record>> m_records;
        QVector<RmRecord> m_removed;

        void initRecords();
    public:
        LineBaseFileManager() = default;
    };

} // namespace awf