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
    _this->message(QString("setFile: analysis completed for %1").arg(filePath));
    return PSCVar::null();
}

PSCVar builtin_getDef(const QVector<PSCVar>& args, PSCOperator* _this) {
    auto& data = _this->context->cppContext.result;
    using Identifier = CppCodeAnalyzerResult::Identifier;
    Identifier current = data.getRoot();
    for (const PSCVar& arg : args) {
        QString name = arg.data();
        bool found = false;
        for (Identifier childId : data.children[current]) {
            if (data.name.value(childId) == name) {
                current = childId;
                found = true;
                break;
            }
        }
        if (!found) {
            _this->error(QString("Identifier '%1' not found in current scope").arg(name));
            return PSCVar::null();
        }
    }
    if (data.def.contains(current)) {
        const auto& snippet = data.def[current];
        auto id = _this->context->getID();
        _this->context->cppContext.savedSnippet[id] = snippet;
        _this->context->cppContext.savedSnippetFilePath[id] = data.filePath[snippet.file];
        return PSCVar(id, PSCVarType::codeSnippet);
    }
    if (data.decl.contains(current)) {
        const auto& snippet = data.decl[current];
        auto id = _this->context->getID();
        _this->context->cppContext.savedSnippet[id] = snippet;
        _this->context->cppContext.savedSnippetFilePath[id] = data.filePath[snippet.file];
        return PSCVar(id, PSCVarType::codeSnippet);
    }
    _this->warn(QString("No definition or declaration found for '%1'")
        .arg(data.name.value(current, "?")));
    return PSCVar::null();
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
