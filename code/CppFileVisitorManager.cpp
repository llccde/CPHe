#include "CppFileVisitorManager.h"
#include <QByteArray>
#include <QDebug>

// 供 clang_visitChildren 使用的收集器
CXChildVisitResult CppFileVisitorManager::collectChildren(CXCursor child, CXCursor /*parent*/, CXClientData data) {
    auto* children = static_cast<QVector<CXCursor>*>(data);
    children->push_back(child);
    return CXChildVisit_Continue;
}

CppFileVisitorManager::CppFileVisitorManager(const QString& mainFile,
    const QVector<QString>& includeFiles,
    const QVector<QString>& includeFolders)
    : ctx{ CodeSnippet(clang_getNullCursor(), -1, m_pool, 0, 0, 0, 0), {} }
    , m_index(clang_createIndex(0, 0))
    , m_tu(nullptr)
{
    // 构造编译参数
    QVector<const char*> args;
    args.push_back("-x");
    args.push_back("c++");

    for (const QString& folder : includeFolders) {
        QByteArray arg = ("-I" + folder).toUtf8();
        args.push_back(arg.constData());
    }
    for (const QString& file : includeFiles) {
        QByteArray arg = ("-include" + file).toUtf8();
        args.push_back(arg.constData());
    }

    m_tu = clang_parseTranslationUnit(
        m_index,
        mainFile.toUtf8().constData(),
        args.data(), static_cast<int>(args.size()),
        nullptr, 0,
        CXTranslationUnit_None
    );

    if (!m_tu) {
        qWarning() << "Failed to parse translation unit:" << mainFile;
        return;
    }

    // 初始化根节点
    CXCursor tuCursor = clang_getTranslationUnitCursor(m_tu);
    CXSourceRange range = clang_getCursorExtent(tuCursor);
    CXSourceLocation start = clang_getRangeStart(range);
    CXSourceLocation end = clang_getRangeEnd(range);

    unsigned startLine = 0, startCol = 0, endLine = 0, endCol = 0;
    clang_getSpellingLocation(start, nullptr, &startLine, &startCol, nullptr);
    clang_getSpellingLocation(end, nullptr, &endLine, &endCol, nullptr);

    ID fileID = getLocationFileID(tuCursor);
    m_rootSnippet = CodeSnippet(tuCursor, fileID, m_pool,
        static_cast<int>(startLine), static_cast<int>(startCol),
        static_cast<int>(endLine), static_cast<int>(endCol));

    ctx.currentCursor = m_rootSnippet;
    ctx.parentStack.clear();
}

CppFileVisitorManager::~CppFileVisitorManager() {
    if (m_tu) {
        clang_disposeTranslationUnit(m_tu);
    }
    if (m_index) {
        clang_disposeIndex(m_index);
    }
}

ID CppFileVisitorManager::getLocationFileID(CXCursor cursor) {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned line, column, offset;
    clang_getSpellingLocation(location, &file, &line, &column, &offset);
    CXString fileName = clang_getFileName(file);
    QString qFileName = QString::fromUtf8(clang_getCString(fileName));
    clang_disposeString(fileName);
    return m_pool.save(qFileName);
}

// 核心递归：无参，从 ctx 获取当前节点，callback 由参数传入
void CppFileVisitorManager::visitCursor(const CallBack& callback) {
    // 当前游标和父游标
    CXCursor cursor = ctx.currentCursor.cursor;
    CXCursor parentCursor = ctx.parentStack.isEmpty() ? clang_getNullCursor()
        : ctx.parentStack.top().cursor;

    // 1. 调用用户回调
    if (callback) {
        callback(*this);
    }

    if (stopChildren) {
        stopChildren = false;
        return;
    }
    // 2. 若回调请求停止，直接返回，保留当前 ctx
    if (stopVisit)
        return;

    // 3. 收集子节点
    QVector<CXCursor> children;
    clang_visitChildren(cursor, collectChildren, &children);

    // 4. 递归处理每个子节点
    for (const auto& child : children) {
        if (stopVisit)
            break;

        // 为子节点构建 CodeSnippet
        CXSourceRange range = clang_getCursorExtent(child);
        CXSourceLocation start = clang_getRangeStart(range);
        CXSourceLocation end = clang_getRangeEnd(range);

        unsigned startLine, startCol, endLine, endCol;
        clang_getSpellingLocation(start, nullptr, &startLine, &startCol, nullptr);
        clang_getSpellingLocation(end, nullptr, &endLine, &endCol, nullptr);

        ID fileID = getLocationFileID(child);
        CodeSnippet childSnippet(child, fileID, m_pool,
            static_cast<int>(startLine), static_cast<int>(startCol),
            static_cast<int>(endLine), static_cast<int>(endCol));

        // 压栈，切换当前上下文
        ctx.parentStack.push(ctx.currentCursor);
        ctx.currentCursor = childSnippet;

        visitCursor(callback);   // 递归
        if (stopVisit) {
            return;
        }
        ctx.currentCursor = ctx.parentStack.pop();
    
    }
}

CppFileVisitorManager::Context CppFileVisitorManager::visitAST(CallBack callback) {
    if (!callback || !m_tu) return ctx;
    auto old = ctx;
    stopChildren = false;
    stopVisit = false;
    visitCursor(callback);
    return old;
}

void CppFileVisitorManager::stopTraversalAll() {
    stopVisit = true;
}

void CppFileVisitorManager::stopCurrentChildren()
{
    stopChildren = true;
}

CppFileVisitorManager::Context CppFileVisitorManager::gotoCodeSnippet(const CodeSnippet& snippet,bool& done) {
    Context old = ctx; 

    ctx.currentCursor = m_rootSnippet;
    ctx.parentStack.clear();
    bool found = false;
    visitAST([&](CppFileVisitorManager& mgr) {
        if (mgr.ctx.currentCursor.include(snippet)) {
            if (mgr.ctx.currentCursor.same(snippet)) {
                mgr.stopTraversalAll();
                found = true;
            }
        }
        else{
            mgr.stopCurrentChildren();
        }
    });
    done = found;
    return old;

}

bool CppFileVisitorManager::gotoAndDo(const CodeSnippet& snippet, CallBack callback)
{
    bool s;
    auto old = ctx;
    gotoCodeSnippet(snippet, s);
    if(s) visitAST(callback);
    recoverContext(old);
    return s;
}

void CppFileVisitorManager::recoverContext(const Context& saved) {
    ctx = saved;
}