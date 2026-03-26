#include "CodeAnalyzer.h"
#include <clang-c/Index.h>
#include<functional>
#include<iostream>
CodeAnalyzer::NameMapNode CodeAnalyzer::NameMapNode::unAvailableNode("", CodeAnalyzer::NameMapNode::unAvailable);

template <class T> 
using delType = std::function<void(T*)>;
template<class T>
using UniqueResOf = std::unique_ptr<T,delType<T>>;
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
		[](CXCursor cursor, CXCursor /*parent*/, CXClientData /*clientData*/) {
			// 获取游标的显示名称（如函数名、变量名、类名等）
			UniqueResOf<CXString> name(new CXString, [](CXString* s) {
					clang_disposeString(*s);
				});
			*name = clang_getCursorSpelling(cursor);

			const char* cname = clang_getCString(*name);

			if (cname && *cname) {                     // 只输出有名称的节点
				std::cout << cname << std::endl;
			}
			// 继续遍历子节点
			return CXChildVisit_Recurse;
		},
		nullptr
	);
	
	
}

uniqueCharArray charArrayFromQString(const QString s){
	QByteArray ba = s.toUtf8();
	auto data = std::make_unique<char[]>(ba.size() + 1);
	std::strcpy(data.get(), ba.constData());
	return data;
};