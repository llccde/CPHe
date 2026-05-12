#pragma once
#include <clang-c/Index.h>
#include <QVector>
#include <stdexcept>
#include"CodeSnippet.h"
namespace lct {

    // 自定义异常类，用于替代 RunTimeErrorCollector 的错误记录
    class LCTError : public std::runtime_error {
    public:
        //explicit LCTError(const std::string& msg) : std::runtime_error(msg) {}
        explicit LCTError(const QString& msg) : std::runtime_error(msg.toUtf8().constData()) {}
    };

    using CodeSnippetRange = CodeRnage;

    struct VisitorClientData {
        QVector<CXCursor>* cursors = nullptr;
    };

    // 访客回调（声明为普通函数，不再使用 static）
    CXChildVisitResult getDirectDeclChildNodeVisitor(
        CXCursor cursor, CXCursor parent, CXClientData client_data);

    // 获取直接声明子节点（失败时抛出 LCTError）
    QVector<CXCursor> getDirectDeclChildNode(CXCursor root);

    // 获取代码片段范围（失败时抛出 LCTError）
    CodeSnippetRange getCodeSnippetRange(CXCursor cursor);

} // namespace lct