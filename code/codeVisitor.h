#pragma once
#include"CodeAnalyzer.h"
#include"helpfulTypes.h"
#include <clang-c/Index.h>
#include<iostream>
#include<qdialog.h>
#include <QMessageBox>
#include<qstring.h>
#include<string>
static UniqueResOf<CXString> warpCXString(CXString cs) {

	UniqueResOf<CXString> name(new CXString, [](CXString* s) {
		clang_disposeString(*s);
		});
	*name = cs;
	return std::move(name);
};

class CodeScope {
public:
    using CodeLocation = unsigned int;
    
    QString fileBeginName;
    QString fileEndName;
    CodeLocation rowBegin = 0;
    CodeLocation columnBegin = 0;
    CodeLocation rowEnd = 0;
    CodeLocation columnEnd = 0;

    CodeScope() = default;

    explicit CodeScope(const CXCursor& cursor) {
        CXSourceRange extent = clang_getCursorExtent(cursor);
        CXSourceLocation startLocation = clang_getRangeStart(extent);
        CXSourceLocation endLocation = clang_getRangeEnd(extent);

        CXFile file1 = nullptr, file2 = nullptr;
        CodeLocation offset1 = 0, offset2 = 0;

        clang_getSpellingLocation(startLocation, &file1, &rowBegin, &columnBegin, &offset1);
        clang_getSpellingLocation(endLocation, &file2, &rowEnd, &columnEnd, &offset2);

        fileBeginName = warpCXString(clang_getFileName(file1));
        fileEndName = warpCXString(clang_getFileName(file2));
    }

    // 相等比较
    bool operator==(const CodeScope& other) const {
        return fileBeginName == other.fileBeginName &&
            fileEndName == other.fileEndName &&
            rowBegin == other.rowBegin &&
            columnBegin == other.columnBegin &&
            rowEnd == other.rowEnd &&
            columnEnd == other.columnEnd;
    }

    bool contains(const CodeScope& other) const {
        if (this->isCrossFileScope() || other.isCrossFileScope()) {
            return false; 
        }
        if (fileBeginName != other.fileBeginName) {
            return false;
        }
        bool startInside = (rowBegin < other.rowBegin) ||
            (rowBegin == other.rowBegin && columnBegin <= other.columnBegin);
        bool endInside = (rowEnd > other.rowEnd) ||
            (rowEnd == other.rowEnd && columnEnd >= other.columnEnd);
        return startInside && endInside;
    }
    bool isCrossFileScope() const {
        if (fileBeginName != fileEndName) {
            return true;
        }
    }

    // 转换为可读字符串
    QString toString() const {
        return QString("%1:[%2,%3] to %4:[%5,%6]")
            .arg(fileBeginName)
            .arg(rowBegin).arg(columnBegin)
            .arg(fileEndName)
            .arg(rowEnd).arg(columnEnd);
    }
    ~CodeScope() {};
private:
    // 辅助函数：将 CXString 转换为 QString
    static QString warpCXString(CXString str) {
        QString result = QString::fromUtf8(clang_getCString(str));
        clang_disposeString(str);
        return result;
    }
};

class Identifier {
public:
    CXCursorKind kind;
    QString name;
    Identifier(CXCursor cursor) {
        auto name = warpCXString(clang_getCursorSpelling(cursor));
        const char* cname = clang_getCString(*name);
        this->name = QString(cname);
        this->kind = clang_getCursorKind(cursor);
    }

};
class CodeAnalyzer::Visitor {
	CodeAnalyzer* outer;
public:
	CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor) {
		auto name = warpCXString(clang_getCursorSpelling(cursor));
		const char* cname = clang_getCString(*name);
        std::string stdStr;
		if (cname && *cname) {
			std::cout << cname;
			auto kind = clang_getCursorKind(cursor);
			if (clang_isDeclaration(kind)) {
                std::cout<<"\t\t" << CodeScope(cursor).toString().toStdString();
                
                
                    
			}
			
			std::cout << std::endl;
		}

		return CXChildVisit_Recurse;
	};
	bool isTypeDeclaration(CXCursor cursor) {
		CXCursorKind kind = clang_getCursorKind(cursor);
		switch (kind) {
			// 结构体、类、联合体、枚举
		case CXCursor_StructDecl:
		case CXCursor_ClassDecl:
		case CXCursor_UnionDecl:
		case CXCursor_EnumDecl:

			// 类型别名
		case CXCursor_TypedefDecl:          // C typedef
		case CXCursor_TypeAliasDecl:        // C++11 using alias

			// 模板相关类型声明
		case CXCursor_ClassTemplate:                // 类模板
		case CXCursor_ClassTemplatePartialSpecialization: // 类模板偏特化
		case CXCursor_TemplateTypeParameter:        // 模板类型参数
		case CXCursor_TemplateTemplateParameter:    // 模板模板参数
		case CXCursor_TypeAliasTemplateDecl:        // C++14 类型别名模板

			// 其他类型相关声明
		case CXCursor_NamespaceAlias:        // 命名空间别名（也算类型域）
		case CXCursor_Namespace:              // 命名空间（虽然不是严格类型，但常一起处理）
			return true;

		default:
			return false;
		}
	}
};
