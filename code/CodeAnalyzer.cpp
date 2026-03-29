#include "CodeAnalyzer.h"
#include <clang-c/Index.h>
#include<functional>
#include<iostream>
#include"helpfulTypes.h"
#include"codeVisitor.h"
CodeAnalyzer::NameMapNode CodeAnalyzer::NameMapNode::unAvailableNode("", CodeAnalyzer::NameMapNode::unAvailable);


static void printDiagnostics(CXTranslationUnit tu) {
	unsigned numDiags = clang_getNumDiagnostics(tu);
	for (unsigned i = 0; i < numDiags; ++i) {
		CXDiagnostic diag = clang_getDiagnostic(tu, i);
		CXString msg = clang_getDiagnosticSpelling(diag);
		std::cout << "Diagnostic: " << clang_getCString(msg) << std::endl;
		clang_disposeString(msg);
		clang_disposeDiagnostic(diag);
	}
}

CodeAnalyzer::CodeAnalyzer():_visitor(new Visitor())
{
	
}
void CodeAnalyzer::parseNames(){
	int size = files.size();
	// 默认将文件全加载进内存, 方便管理
	auto filesCache = std::make_unique<CXUnsavedFile[]>(size);
	Vector<ContentRes> contents(size);
	Vector<uniqueCharArray> paths(size);

	for (int i = 0; i < size; i++){
		contents[i] = files[i].get()->getContent();
		paths[i] = files[i].get()->getFullPathRaw();
		filesCache[i] = { paths[i].get(),contents[i]->getCharArray(),contents[i]->length() };
	}

	UniqueResOf<CXIndex> index(new CXIndex, [](CXIndex* i) {
			if (i&&*i) {
				clang_disposeIndex(*i);
				delete(i);
			}
			return;});
	*index = clang_createIndex(0, 1);
	UniqueResOf<CXTranslationUnit> tu(new CXTranslationUnit, [](CXTranslationUnit* tu) {
		if (tu && *tu) {
			clang_disposeTranslationUnit(*tu);
			delete(tu);
		}
		return;});
	 clang_parseTranslationUnit2(
		*index,
		paths[getMainIndex()].get(),
		nullptr, 0,              // 命令行参数（可空）
		filesCache.get(), size,        // 提供 unsaved files 数组
		CXTranslationUnit_None,tu.get()
	);
	 if (!*tu) {
		 printDiagnostics(*tu);
	 }
	CXCursor cursor = clang_getTranslationUnitCursor(*tu);
	clang_visitChildren(
		cursor,
		[](CXCursor cursor, CXCursor p2, CXClientData _this) {
			return static_cast<CodeAnalyzer::Visitor*>(_this)->cursorVisitor(cursor, p2);
		},
		static_cast<void*>(this->_visitor.get())
	);
	
	
}

CodeAnalyzer::~CodeAnalyzer()
{
}


