#pragma once

#include <QMap>
#include <QString>
#include <QVector>
#include <clang-c/Index.h>
#include<qdebug.h>
struct CppCodeAnalyzerResult {
    using Identifier = unsigned long long int;
    using fileID = unsigned long long int;
    Identifier getRoot() {
        return 0;
    };
    struct CodeSnippet {
        fileID file = 0;
        long long beginLine = 0;
        long long beginColumn = 0;
        long long endLine = 0;
        long long endColumn = 0;
    };

    QMap<Identifier, QString> name;       // ID -> 名称
    QMap<Identifier, QString> USRID;      // ID -> USR
    QMap<Identifier, QVector<Identifier>> children; // ID -> 子节点ID列表
    QMap<Identifier, Identifier> parent;  // ID -> 父节点ID
    QMap<Identifier, CodeSnippet> decl;   // ID -> 声明位置
    QMap<Identifier, CodeSnippet> def;    // ID -> 定义位置
    QMap<fileID, QString> filePath;       // fileID -> 文件路径
    void print() const;
};

class CppCodeAnalyzer {
public:
    void addIncludeFile(QString path);
    void addIncludeFolder(QString path);
    CppCodeAnalyzerResult runAnalyzer(QString mainFile);

private:
    QStringList m_includeFiles;
    QStringList m_includeFolders;
};