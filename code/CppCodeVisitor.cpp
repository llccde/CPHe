#include "CppCodeVisitor.h"
#include <cassert>
#include <iostream>
#include <QMessageBox>
#include"CodeAnalyzer.h"
#include <clang-c/Index.h>

// 辅助函数实现

inline QString CXStringToQString(CXString str) {
    QString result = QString::fromUtf8(clang_getCString(str));
    clang_disposeString(str);
    return result;
}

// QStringPool 实现
poolID QStringPool::add(const QString& s) {
    for (size_t i = 0; i < strings.size(); ++i) {
        if (strings[i] == s) return static_cast<poolID>(i);
    }
    strings.push_back(s);
    return static_cast<poolID>(strings.size() - 1);
}

QString& QStringPool::get(poolID i) {
    return strings[i];
}

// CodeScope 实现
CodeScope::CodeScope(const CXCursor& cursor, QStringPool* pool)
    : FileNamePool(pool) {
    CXSourceRange extent = clang_getCursorExtent(cursor);
    CXSourceLocation startLocation = clang_getRangeStart(extent);
    CXSourceLocation endLocation = clang_getRangeEnd(extent);

    CXFile file1 = nullptr, file2 = nullptr;
    CodeLocation offset1 = 0, offset2 = 0;

    clang_getSpellingLocation(startLocation, &file1, &rowBegin, &columnBegin, &offset1);
    clang_getSpellingLocation(endLocation, &file2, &rowEnd, &columnEnd, &offset2);

    fileBeginName = FileNamePool->add(CXStringToQString(clang_getFileName(file1)));
    fileEndName = FileNamePool->add(CXStringToQString(clang_getFileName(file2)));
}

bool CodeScope::operator==(const CodeScope& other) const {
    return fileBeginName == other.fileBeginName &&
        fileEndName == other.fileEndName &&
        rowBegin == other.rowBegin &&
        columnBegin == other.columnBegin &&
        rowEnd == other.rowEnd &&
        columnEnd == other.columnEnd;
}

bool CodeScope::contains(const CodeScope& other) const {
    if (this->isCrossFileScope() || other.isCrossFileScope()) {
        assert(false);
        return false;
    }
    if (fileBeginName != other.fileBeginName) return false;
    bool startInside = (rowBegin < other.rowBegin) ||
        (rowBegin == other.rowBegin && columnBegin <= other.columnBegin);
    bool endInside = (rowEnd > other.rowEnd) ||
        (rowEnd == other.rowEnd && columnEnd >= other.columnEnd);
    return startInside && endInside;
}

bool CodeScope::isCrossFileScope() const {
    return fileBeginName != fileEndName;
}

QString CodeScope::toString() const {
    return QString(" Scope AT %1:[%2,%3] to %4:[%5,%6] ")
        .arg(FileNamePool->get(fileBeginName))
        .arg(rowBegin).arg(columnBegin)
        .arg(FileNamePool->get(fileEndName))
        .arg(rowEnd).arg(columnEnd);
}

QString CodeScope::getFileName() {
    return FileNamePool->get(fileBeginName);
}

// Identifier 实现
Identifier::Identifier(CXCursor cursor) {

    this->name = CXStringToQString(clang_getCursorSpelling(cursor));
    this->kind = clang_getCursorKind(cursor);
}

QString Identifier::toString() {
    return QString(" %1,type:%2").arg(name).arg(
        CXStringToQString(clang_getCursorKindSpelling(kind))
    );
}

bool Identifier::isDecl() {
    return clang_isDeclaration(kind);
}

bool Identifier::hasName() {
    return name.length() > 0;
}

// CodeNode 实现
CodeNode::CodeNode() : isRoot(true) {}

CodeNode CodeNode::getRoot() {
    return CodeNode();
}

CodeNode::CodeNode(CXCursor cursor, QStringPool* pool) : cs(cursor, pool), id(cursor) {}

QString CodeNode::toString() {
    return QString("CodeNode\t%1\t%2").arg(id.toString()).arg(cs.toString());
}

bool CodeNode::isNameNode() {
    return id.hasName() && id.isDecl();
}

void CodeNode::sinkCallOnRoot(CodeNode*&& node) {
    assert(node != nullptr);
    sink_impl(std::move(node));
}

bool CodeNode::getIsRoot() const {
    return isRoot;
}

void CodeNode::sink_impl(CodeNode*&& node) {
    bool result = false;
    for (auto& child : beContaineds) {
        assert(child.get() != nullptr);
        if (child->cs.contains(node->cs)) {
            child->sink_impl(std::move(node));
            result = true;
            break;
        }
    }
    if (!result) {
        beContaineds.push_back(std::unique_ptr<CodeNode>(node));
    }
}

// CppNameMapNode 实现
CppNameMapNode::CppNameMapNode(QString name, const CodeNode* node, QStringPool* pool)
    : NameMapNode(name, (node->getIsRoot() ? NameMapNode::root : NameMapNode::normal)),pool(pool),
    isRoot(node->getIsRoot()) {
    if (!isRoot) {
        mypos = CodePosition("", node->cs.rowBegin, node->cs.columnBegin,
            node->cs.rowEnd, node->cs.columnEnd);
    }
}

QString CppNameMapNode::getMyFileName() {
    return pool->get(fileID);
}

bool CppNameMapNode::GetIsRoot() {
    return isRoot;
}

void CppNameMapNode::setFile(QString myFileName) {
    assert(!isRoot);
    this->fileID = pool->add(myFileName);
}


CodePosition CppNameMapNode::getPosition() {
    assert(!isRoot);
    CodePosition pos = mypos;
    pos.file = pool->get(fileID);
    return pos;
}

CPPCodeVisitor::CPPCodeVisitor()
    : rootNode(CodeNode::getRoot())
{
}

CXChildVisitResult CPPCodeVisitor::cursorVisitor(CXCursor cursor, CXCursor /*parent*/) {
    auto CN = new CodeNode(cursor, &this->pool);
    if (CN->isNameNode()) {
        std::cout << CN->toString().toStdString() << "\n";
    }
    rootNode.sinkCallOnRoot(std::move(CN));
    return CXChildVisit_Recurse;
}


std::unique_ptr<NameMap> CPPCodeVisitor::getNameMap() {
    CppNameMapNode* root = new CppNameMapNode("", &rootNode,&this->pool);
    for (auto& i : rootNode.beContaineds) {
        preOrderDFS_NameMapBuild(root, i.get());
    }
    return std::unique_ptr<NameMap>(root);
}

void CPPCodeVisitor::preOrderDFS_NameMapBuild(CppNameMapNode* parent, CodeNode* _this) {
    auto childNameNode = parent;
    if (_this->isNameNode()) {
        auto NewNode = new CppNameMapNode(_this->id.name, _this, &this->pool);
        NewNode->setFile(_this->cs.getFileName());
        childNameNode = NewNode;
        parent->add(std::move(NewNode));
    }
    for (auto& i : _this->beContaineds) {
        preOrderDFS_NameMapBuild(childNameNode, i.get());
    }
}