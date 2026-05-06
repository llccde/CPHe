#include "CppFileAnalyzer.h"
#include <cassert>

// 辅助：CXString 转 QString 并自动释放
static QString cxStringToQString(CXString str) {
    QString result = QString::fromUtf8(clang_getCString(str));
    clang_disposeString(str);
    return result;
}

// 获取 USR 字符串（处理空 cursor）
static QString getCursorUSR(CXCursor cursor) {
    if (clang_equalCursors(cursor, clang_getNullCursor()))
        return QString();
    return cxStringToQString(clang_getCursorUSR(cursor));
}

// 提取文件 ID，若不存在则创建
static CppCodeAnalyzerResult::fileID getOrCreateFileID(
    CXFile file,
    QMap<QString, CppCodeAnalyzerResult::fileID>& pathToId,
    CppCodeAnalyzerResult& result,
    CppCodeAnalyzerResult::fileID& nextFileId)
{
    if (!file) return 0;
    QString path = cxStringToQString(clang_getFileName(file));
    auto it = pathToId.find(path);
    if (it != pathToId.end())
        return it.value();

    CppCodeAnalyzerResult::fileID id = nextFileId++;
    pathToId[path] = id;
    result.filePath[id] = path;
    return id;
}

// 从 cursor 填充 CodeSnippet
static CppCodeAnalyzerResult::CodeSnippet makeCodeSnippet(
    CXCursor cursor,
    QMap<QString, CppCodeAnalyzerResult::fileID>& pathToId,
    CppCodeAnalyzerResult& result,
    CppCodeAnalyzerResult::fileID& nextFileId)
{
    CppCodeAnalyzerResult::CodeSnippet snippet;
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXSourceLocation start = clang_getRangeStart(range);
    CXSourceLocation end = clang_getRangeEnd(range);

    CXFile file;
    unsigned line, column, offset;
    clang_getSpellingLocation(start, &file, &line, &column, &offset);
    snippet.file = getOrCreateFileID(file, pathToId, result, nextFileId);
    snippet.beginLine = static_cast<long long>(line);
    snippet.beginColumn = static_cast<long long>(column);

    clang_getSpellingLocation(end, &file, &line, &column, &offset);
    snippet.endLine = static_cast<long long>(line);
    snippet.endColumn = static_cast<long long>(column);
    return snippet;
}

// ------------------- 遍历数据结构 -------------------
struct VisitorData {
    CppCodeAnalyzerResult* result;
    QMap<QString, CppCodeAnalyzerResult::Identifier>* usrToId;   // USR -> ID
    QMap<CppCodeAnalyzerResult::Identifier, QString>* parentUSR; // ID -> 语义父USR
    QMap<QString, CppCodeAnalyzerResult::fileID>* pathToId;      // 路径 -> fileID
    CppCodeAnalyzerResult::fileID* nextFileId;
    CppCodeAnalyzerResult::Identifier* nextId;
};

// ------------------- 遍历回调 -------------------
static CXChildVisitResult visitorCallback(CXCursor cursor, CXCursor /*parent*/, CXClientData clientData) {
    auto* data = static_cast<VisitorData*>(clientData);
    CXCursorKind kind = clang_getCursorKind(cursor);

    // 只处理声明
    if (clang_isDeclaration(kind) == 0)
        return CXChildVisit_Recurse;

    QString usr = getCursorUSR(cursor);
    if (usr.isEmpty())                             // 无 USR（匿名结构体等）忽略
        return CXChildVisit_Recurse;

    CppCodeAnalyzerResult& res = *data->result;
    auto& usrToId = *data->usrToId;
    auto& parentUSR = *data->parentUSR;
    auto& pathToId = *data->pathToId;
    CppCodeAnalyzerResult::fileID& nextFileId = *data->nextFileId;
    CppCodeAnalyzerResult::Identifier& nextId = *data->nextId;

    CppCodeAnalyzerResult::Identifier id;

    auto it = usrToId.find(usr);
    if (it != usrToId.end()) {
        id = it.value();                           // 已存在，只更新定义/声明位置
    }
    else {
        id = nextId++;
        usrToId[usr] = id;
        res.name[id] = cxStringToQString(clang_getCursorSpelling(cursor));
        res.USRID[id] = usr;
    }

    // 获取语义父节点的 USR
    CXCursor semParent = clang_getCursorSemanticParent(cursor);
    QString parentUsrStr;
    if (clang_getCursorKind(semParent) == CXCursor_TranslationUnit) {
        parentUsrStr.clear();                      // 顶层实体 -> root
    }
    else {
        parentUsrStr = getCursorUSR(semParent);
    }
    parentUSR[id] = parentUsrStr;                  // 存储，最后统一建立树

    // 判断定义 / 声明
    bool isDef = clang_isCursorDefinition(cursor);
    CppCodeAnalyzerResult::CodeSnippet snippet = makeCodeSnippet(cursor, pathToId, res, nextFileId);
    if (isDef) {
        res.def[id] = snippet;                    // 定义位置
        // 如果之前只有声明，保持 decl 不变（若有）
    }
    else {
        if (!res.decl.contains(id))               // 第一个声明
            res.decl[id] = snippet;
    }

    return CXChildVisit_Recurse;                  // 继续深入子节点
}

// ------------------- 执行分析 -------------------
CppCodeAnalyzerResult CppCodeAnalyzer::runAnalyzer(QString mainFile) {
    CppCodeAnalyzerResult result;
    result.name[0] = "root";
    result.USRID[0] = "theRootOfAll";

    // 构造编译参数
    QVector<const char*> args;
    args.push_back("-x");
    args.push_back("c++");                          // 强制 C++ 模式
    for (const QString& folder : m_includeFolders) {
        QByteArray arg = ("-I" + folder).toUtf8();
        args.push_back(arg.constData());
    }
    for (const QString& file : m_includeFiles) {
        QByteArray arg = ("-include" + file).toUtf8();
        args.push_back(arg.constData());
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index,
        mainFile.toUtf8().constData(),
        args.data(), args.size(),
        nullptr, 0,
        CXTranslationUnit_None
    );

    if (!tu) {
        clang_disposeIndex(index);
        return result;
    }

    // 第一阶段：收集所有实体
    QMap<QString, CppCodeAnalyzerResult::Identifier> usrToId;
    QMap<CppCodeAnalyzerResult::Identifier, QString> parentUSR;
    QMap<QString, CppCodeAnalyzerResult::fileID> pathToId;
    CppCodeAnalyzerResult::fileID nextFileId = 1;
    CppCodeAnalyzerResult::Identifier nextId = 1;

    VisitorData data{ &result, &usrToId, &parentUSR, &pathToId, &nextFileId, &nextId };
    clang_visitChildren(clang_getTranslationUnitCursor(tu), visitorCallback, &data);

    // 第二阶段：建立父子关系（root id = 0）
    for (auto it = usrToId.begin(); it != usrToId.end(); ++it) {
        CppCodeAnalyzerResult::Identifier id = it.value();
        const QString& pUSR = parentUSR[id];
        CppCodeAnalyzerResult::Identifier parentId = 0;  // 默认为 root

        if (!pUSR.isEmpty()) {
            auto pIt = usrToId.find(pUSR);
            if (pIt != usrToId.end())
                parentId = pIt.value();
        }
        result.parent[id] = parentId;
        result.children[parentId].push_back(id);
    }

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return result;
}

void CppCodeAnalyzer::addIncludeFile(QString path) {
    m_includeFiles.append(path);
}

void CppCodeAnalyzer::addIncludeFolder(QString path) {
    m_includeFolders.append(path);
}

void CppCodeAnalyzerResult::print() const
{
    // 递归遍历树，从 root (ID=0) 开始
    std::function<void(Identifier, int)> printSubtree = [&](Identifier id, int depth) {
        // 缩进：每层两个空格
        QString indent(depth * 2, ' ');
        // 获取当前节点名称，若缺失则显示 "???"
        QString nodeName = name.value(id, QStringLiteral("???"));

        // 输出：去掉引号，按格式输出
        qDebug().noquote() << QStringLiteral("%1%2").arg(indent, nodeName);

        // 输出子节点
        if (children.contains(id)) {
            for (Identifier childId : children[id]) {
                printSubtree(childId, depth + 1);
            }
        }
        };

    if (!name.contains(0)) {
        qDebug() << "Result tree is empty (no root node).";
        return;
    }
    printSubtree(0, 0);
}