#include"PSCBuildingOperators.h"
#include <QFileInfo>
#include <iostream>
#include"PSCVarDrefTool.h"
#include"FileContentManager.h"
#include"CppFileVisitorManager.h"
#include"LibclangTool.h"

// 外部全局/静态对象声明（来自原文件）
extern struct Receivers {
    QString empty, outPut, env;
} recvNames;
extern const QString defaultUDI;   // 如果 defaultUDI 是外部常量

// ---------------------- 操作符实现 ----------------------

PSCVar builtin_join(const QVector<PSCVar>& args, PSCOperator* /*op*/) {
    QString ret;
    for (const auto& arg : args)
        ret.append(arg.data());
    return PSCVar(ret, PSCVarType::rawStr);
}

PSCVar builtin_printOut(const QVector<PSCVar>& /*args*/, PSCOperator* _this) {
    QString out = _this->redCtx(recvNames.outPut, defaultUDI);
    std::cout << out.toStdString() << std::endl;
    return PSCVar::null();
}

PSCVar builtin_setFile(const QVector<PSCVar>& args, PSCOperator* _this) {
    if (args.isEmpty()) {
        _this->error("setFile: missing file path argument");
        return PSCVar::null();
    }
    QString filePath = args[0].data();
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        _this->error(QString("setFile: file does not exist: %1").arg(filePath));
        return PSCVar::null();
    }
    if (!fileInfo.isFile()) {
        _this->error(QString("setFile: path is not a regular file: %1").arg(filePath));
        return PSCVar::null();
    }
    if (!fileInfo.isReadable()) {
        _this->error(QString("setFile: file is not readable: %1").arg(filePath));
        return PSCVar::null();
    }
    if (fileInfo.size() == 0) {
        _this->warn(QString("setFile: file is empty: %1").arg(filePath));
    }

    auto& cpp = _this->context->cppContext;
    cpp.MainFilePath = filePath;
    cpp.result = cpp.analyzer.runAnalyzer(filePath);
    cpp.libclangContext = cpp.analyzer.getContext(filePath);
    _this->message(QString("setFile: analysis completed for %1").arg(filePath));
    return PSCVar::null();
}
PSCVar builtin_getDef(const QVector<PSCVar>& args, PSCOperator* _this) {

    CXTranslationUnit tu = _this->context->cppContext.libclangContext->tu;   // 接口留白
    if (!tu) {
        _this->error("No translation unit available");
        return PSCVar::null();
    }
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

    CXCursor current = rootCursor;
    for (const PSCVar& arg : args) {
        QString targetName = arg.data();

        QVector<CXCursor> children = lct::getDirectDeclChildNode(current);

        bool found = false;
        for (const CXCursor& child : children) {
            CXString nameStr = clang_getCursorSpelling(child);
            QString childName = QString::fromUtf8(clang_getCString(nameStr));
            clang_disposeString(nameStr);

            if (childName == targetName) {
                current = child;
                found = true;
                break;
            }
        }

        if (!found) {
            _this->error(QString("Identifier '%1' not found in current scope")
                .arg(targetName));
            return PSCVar::null();
        }
    }

    // ------------------------------------------------------------------
    // 3. 查找定义或声明：优先取定义，其次取首次声明
    // ------------------------------------------------------------------
    CXCursor targetCursor = clang_getNullCursor();

    // 3.1 尝试获取定义（如函数体、变量初始化等）
    CXCursor defCursor = clang_getCursorDefinition(current);
    if (!clang_Cursor_isNull(defCursor) && !clang_equalCursors(defCursor, current)) {
        // 有些情况下定义游标可能和声明相同，需进一步判断是否有定义位置
        targetCursor = defCursor;
    }

    // 3.2 如果没有有效定义，使用声明游标
    if (clang_Cursor_isNull(targetCursor)) {
        // 如果是声明但无定义，可以使用 clang_getCanonicalCursor 或直接使用 current
        targetCursor = current;   // current 此时就是路径最后匹配到的声明节点
    }

    // ------------------------------------------------------------------
    // 4. 使用 lct::getCodeSnippetRange 提取代码范围
    // ------------------------------------------------------------------
    lct::CodeSnippetRange snippet;
    try {
        snippet = lct::getCodeSnippetRange(targetCursor);
    }
    catch (const lct::LCTError& e) {
        _this->error(QString("Failed to get code snippet: %1").arg(e.what()));
        return PSCVar::null();
    }

    // ------------------------------------------------------------------
    // 5. 生成 ID，将 snippet 及其文件路径保存到上下文中
    //    （假设 PSCOperator::context->cppContext 提供 savedSnippet / savedSnippetFilePath 容器）
    // ------------------------------------------------------------------
    auto id = _this->context->getID();
    _this->context->cppContext.savedSnippet[id] = snippet;
    // 根据 snippet.file 获取文件全路径（留白：假设 cppContext 有 filePath 映射或直接使用 snippet.file）
    _this->context->cppContext.savedSnippetFilePath[id] =
        _this->context->cppContext.resolveFilePath(snippet.file);      // 接口留白

    return PSCVar(id, PSCVarType::codeSnippet);
}
FileContentManager getDefDepth_impl(CXCursor cx, int dep, PSCOperator* op) {
    RunTimeErrorCollector rc;
    FileContentManager fcm;
    auto rootrange = lct::getCodeSnippetRange(cx);
    fcm.accept(rootrange);
    QVector<CXCursor> lastLevel = {cx};
    for (size_t i = 1; i <= dep; i++)
    {
        QVector<CXCursor> levelIChildren;
        for (auto& c: lastLevel)
        {
            auto children = lct::getDirectDeclChildNode(c);
            for (auto& child:children)
            {
                levelIChildren.append(child);
            }
        }
        lastLevel = std::move(levelIChildren);
    }
    for (auto& i:lastLevel)
    {
        fcm.removeAccept(lct::getCodeSnippetRange(i));
    }
    return fcm;
}
PSCVar builtin_getDefDepth(const QVector<PSCVar>& args, PSCOperator* op)
{
    try
    {
        QString mainFile = op->context->cppContext.MainFilePath;
        bool ok;
         
        int depth = args[0].data().toInt(&ok);
        CXCursor cx = op->context->cppContext.savedCursor[args[1].data()];

        auto& cppctx = op->context->cppContext;
        if (!ok)op->error("first arg should be int");
        auto fcm = getDefDepth_impl(cx, depth, op);
        QFile f(mainFile);
        QVector<QString>rawFile;
        while (!f.atEnd())
        {
            rawFile.append(QString(f.readLine()));
        }
        return PSCVar(fcm.readFile(mainFile,rawFile).join("\n"), PSCVarType::rawStr);
    }
    catch (const std::runtime_error&err)
    {
        op->error(err.what());
        return PSCVar::null();
    }
   
    

}
