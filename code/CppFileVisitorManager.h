#pragma once

#include <clang-c/Index.h>
#include <QString>
#include <QVector>
#include <QStack>
#include <functional>
#include "CodeSnippet.h"
#include<qmenu.h>
#include<qmetatype.h>
#include<qmetaobject.h>
class FileVisitorTag {
    QString tag;
    QVector<QString> adds;
public:
    FileVisitorTag(QString tag, QVector<QString>add) :tag(tag),adds(add){};
    FileVisitorTag(QString tag) :tag(tag), adds({}) {};
    FileVisitorTag(const char tag[]) :tag(tag), adds({}) {};
    operator QString() {
        return tag;
    }
};
class FileVisitorTags:public QObject {
public:
    enum Builtin {
        notVisitChildren,
        notVisitThis
    };
    Q_ENUM(Builtin);
    QMetaEnum type = QMetaEnum::fromType<Builtin>();
    bool isBuiltIn(QString s) {
        bool successful = false;
        type.keyToValue(s.toUtf8().data(), &successful);
        return successful;
    }
    Builtin getBuiltin(QString s) {
        return static_cast<Builtin>(type.keyToValue(s.toUtf8().data()));
    }
    QString getStr(Builtin b) {
        return QString(type.valueToKey(b));
    }
};
class CppFileVisitorManager {
public:
    struct Context {
        CodeSnippet currentCursor;
        QStack<CodeSnippet> parentStack;   // top 是直接父节点

    } ctx;

    using CallBack = std::function<void(CppFileVisitorManager& manager)>;

    CppFileVisitorManager(const QString& mainFile,
        const QVector<QString>& includeFiles = {},
        const QVector<QString>& includeFolders = {});

    ~CppFileVisitorManager();

    CppFileVisitorManager(const CppFileVisitorManager&) = delete;
    CppFileVisitorManager& operator=(const CppFileVisitorManager&) = delete;

    // 遍历 AST，调用 callback 处理每个节点
    Context visitAST(CallBack callback);

    void stopTraversalAll();
    void stopCurrentChildren();
    // 将上下文定位到包含 snippet 的最深层节点，返回旧上下文
    Context gotoCodeSnippet(const CodeSnippet& snippet, bool& done);
    bool gotoAndDo(const CodeSnippet& snippet, CallBack callback);
    // 恢复之前保存的上下文
    void recoverContext(const Context& saved);

    bool isValid() const { return m_tu != nullptr; }

private:
    bool stopVisit = false;
    bool stopChildren = false;
    StringPool m_pool;
    CXIndex m_index;
    CXTranslationUnit m_tu;
    CodeSnippet m_rootSnippet;   // 翻译单元根节点快照

    ID getLocationFileID(CXCursor cursor);
    void visitCursor(const CallBack& callback);   // 递归遍历核心

    // 静态辅助：收集子游标
    static CXChildVisitResult collectChildren(CXCursor child, CXCursor /*parent*/, CXClientData data);
};