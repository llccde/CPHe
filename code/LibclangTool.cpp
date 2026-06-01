#include "LibclangTool.h"

namespace lct {

    CXChildVisitResult getDirectDeclChildNodeVisitor(CXCursor cursor, CXCursor parent,
        CXClientData client_data) {
        auto* data = static_cast<VisitorClientData*>(client_data);
        QVector<CXCursor>* result = data->cursors;

        if (clang_isDeclaration(clang_getCursorKind(cursor))) {
            if (clang_equalCursors(clang_getCursorSemanticParent(cursor), parent)) {
                result->append(cursor);
            }
        }
        return CXChildVisit_Continue;
    }

    QVector<CXCursor> getDirectDeclChildNode(CXCursor root) {
        QVector<CXCursor> result;
        VisitorClientData clientData{ &result };

        unsigned visitResult = clang_visitChildren(
            root, getDirectDeclChildNodeVisitor, &clientData);
        if (visitResult != 0) {
            // 原 rc.putError 改为抛出异常
            throw LCTError("Failed to visit children of the given cursor");
        }
        return result;
    }

    CodeSnippetRange getCodeSnippetRange(CXCursor cursor) {
        CodeSnippetRange range;

        // 检查 cursor 是否有效
        if (clang_Cursor_isNull(cursor)) {
            throw LCTError("Cannot get code snippet range: cursor is null");
        }

        CXSourceRange extent = clang_getCursorExtent(cursor);
        if (clang_Range_isNull(extent)) {
            throw LCTError("Cursor extent is null");
        }

        CXSourceLocation startLoc = clang_getRangeStart(extent);
        CXSourceLocation endLoc = clang_getRangeEnd(extent);

        CXFile file;
        unsigned startLine, startColumn, endLine, endColumn;

        clang_getFileLocation(startLoc, &file, &startLine, &startColumn, nullptr);
        if (!file) {
            throw LCTError("Failed to get file location for start of cursor extent");
        }

        CXString fileName = clang_getFileName(file);
        range.file = QString::fromUtf8(clang_getCString(fileName));
        clang_disposeString(fileName);

        range.row = static_cast<int>(startLine);
        range.column = static_cast<int>(startColumn);

        clang_getFileLocation(endLoc, nullptr, &endLine, &endColumn, nullptr);
        range.rowEnd = static_cast<int>(endLine);
        range.columnEnd = static_cast<int>(endColumn);

        // 原警告忽略：如果结束位置无效，保持 range.re/ce 为 0，不抛出异常
        return range;
    }

} // namespace lct