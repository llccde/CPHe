#pragma once
#include"qstring.h"
#include"qmap.h"
#include<clang-c/Index.h>
using ID = long long;
class StringPool {
public:
    QMap<ID, QString> saved;
    QMap<QString, ID> has;

    const QString& get(ID id) const { return saved[id]; }
    ID save(const QString& data) {
        if (has.contains(data)) return has[data];
        ID newID = saved.size();
        saved[newID] = data;
        has[data] = newID;
        return newID;
    }
};
struct CodeSnippet {
    CXCursor cursor;
    ID filePath;
    StringPool* pool;
    int row, column, rowEnd, columnEnd;

    CodeSnippet(const CXCursor& cursor, ID filePath, StringPool& pool,
        int row, int column, int rowEnd, int columnEnd)
        : filePath(filePath), pool(&pool),
        row(row),cursor(cursor), column(column), rowEnd(rowEnd), columnEnd(columnEnd) {
    }

    const QString& filePathStr() const { return pool->get(filePath); }

    bool include(const CodeSnippet& other) const {
        if (filePathStr() != other.filePathStr()) return false;
        return (row < other.row || (row == other.row && column <= other.column)) &&
            (rowEnd > other.rowEnd || (rowEnd == other.rowEnd && columnEnd >= other.columnEnd));
    }
    bool same(const CodeSnippet& other) const{
        return this->filePathStr() == other.filePathStr()
            && this->column == other.column
            && this->row == other.row
            && this->rowEnd == other.rowEnd
            && this->columnEnd == other.columnEnd;
    }

    CodeSnippet() = default;
};
struct CodeRnage {
    QString file;
    int row, column, rowEnd, columnEnd;

    CodeRnage() = default;
    CodeRnage(const QString& file, int row, int column, int rowEnd, int columnEnd)
        : file(file), row(row), column(column), rowEnd(rowEnd), columnEnd(columnEnd)
    {
    }

    bool operator==(const CodeRnage& other) const
    {
        return file == other.file && row == other.row && column == other.column && rowEnd == other.rowEnd && columnEnd == other.columnEnd;
    }
};