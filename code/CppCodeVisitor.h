#pragma once
#include"CodeAnalyzer.h"
#include "helpfulTypes.h"
#include "NameTree.h"
#include <QString>
#include <memory>
#include <vector>
#include <clang-c/Index.h>
#include<BaseCodeVisitor.h>
// 辅助函数声明

inline QString CXStringToQString(CXString str);

// 字符串池
using poolID = int;
class QStringPool {
    std::vector<QString> strings;
public:
    poolID add(const QString& s);
    QString& get(poolID i);
};

// 前向声明
struct CodeNode;

// 代码范围
class CodeScope {
    friend CodeNode;
public:
    using CodeLocation = unsigned int;
    QStringPool* FileNamePool = nullptr;
    poolID fileBeginName = 0;
    poolID fileEndName = 0;
    CodeLocation rowBegin = 0;
    CodeLocation columnBegin = 0;
    CodeLocation rowEnd = 0;
    CodeLocation columnEnd = 0;

    explicit CodeScope(const CXCursor& cursor, QStringPool* pool);
    bool operator==(const CodeScope& other) const;
    bool contains(const CodeScope& other) const;
    bool isCrossFileScope() const;
    QString toString() const;
    QString getFileName();

    ~CodeScope() = default;
private:
    CodeScope() = default;
};

// 标识符
class Identifier {
    friend CodeNode;
public:
    CXCursorKind kind;
    QString name;

    explicit Identifier(CXCursor cursor);
    QString toString();
    bool isDecl();
    bool hasName();
private:
    Identifier() = default;
};

// AST节点
struct CodeNode {
private:
    bool isRoot = false;
    CodeNode();  // 仅用于构造根节点
public:
    CodeScope cs;
    Identifier id;
    std::vector<std::unique_ptr<CodeNode>> beContaineds;

    static CodeNode getRoot();
    explicit CodeNode(CXCursor cursor, QStringPool* pool);
    QString toString();
    bool isNameNode();
    void sinkCallOnRoot(CodeNode*&& node);
    bool getIsRoot() const;

private:
    void sink_impl(CodeNode*&& node);
};

// 名称映射节点（用于输出）
class CppNameMapNode : public NameMapNode {

    CodePosition mypos;
    poolID fileID;
    QStringPool* pool;
    bool isRoot = false;
    

public:
    explicit CppNameMapNode(QString name, const CodeNode* node, QStringPool*);
    QString getMyFileName();
    bool GetIsRoot();
    void setFile(QString myFileName);
    CodePosition getPosition() override;
};

// ==================== CodeAnalyzer::BaseVisitor 类拆分 ====================
class CPPCodeVisitor : public BaseVisitor {
public:
    CPPCodeVisitor();
    ~CPPCodeVisitor() override = default;

    CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent) override;
    std::unique_ptr<NameMap> getNameMap() override;

private:
    CodeAnalyzer* outer = nullptr;          
    CodeNode rootNode;   
    QStringPool pool;  

    void preOrderDFS_NameMapBuild(CppNameMapNode* parent, CodeNode* _this);
};