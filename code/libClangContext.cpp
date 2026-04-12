#include "libClangContext.h"
#include <clang-c/Index.h>
#include<iostream>
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

LibClangIndexAndTranslationUnit LibClangContextResPack::getUnit()
{
	return LibClangIndexAndTranslationUnit(this);
}

LibClangContextResPack::LibClangContextResPack(int size,int main):
	mainIndex(main),
	fileNum(size),
	contents(size),
	paths(size),
	unsaveFilesCache(std::make_unique<CXUnsavedFile[]>(size))
{
	
}
LibClangContextResPack::~LibClangContextResPack()
{
}

std::unique_ptr<LibClangContextResPack> LibClangContext::getFileCache()
{
	int size = files.size();
	LibClangContextResPack* cache = new LibClangContextResPack(size,getMainIndex());
	for (int i = 0; i < size; i++) {
		cache->contents[i] = files[i].get()->getContent();
		cache->paths[i] = files[i].get()->getFullPathRaw();

		cache->unsaveFilesCache[i] = { 
			cache->paths[i].get(),
			cache->contents[i]->getCharArray(),
			cache->contents[i]->length() 
		};
	}
	cache->fileNum = size;
	cache->mainIndex = getMainIndex();
	return std::unique_ptr<LibClangContextResPack>(std::move(cache));
}

LibClangIndexAndTranslationUnit::LibClangIndexAndTranslationUnit(LibClangIndexAndTranslationUnit&& other) noexcept
{
	this->tu = std::move(other.tu);
	this->index = std::move(other.index);
}

LibClangIndexAndTranslationUnit::LibClangIndexAndTranslationUnit(LibClangContextResPack* data)
{
	UniqueResOf<CXIndex> index(new CXIndex, [](CXIndex* i) {
		if (i && *i) {
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
	std::vector<const char*> args;
	args.push_back("-x");
	args.push_back("c++");
	clang_parseTranslationUnit2(
		*index,
		data->paths[data->mainIndex].get(),
		args.data(), args.size(),              // 命令行参数（可空）
		data->unsaveFilesCache.get(), data->fileNum,        // 提供 unsaved files 数组
		CXTranslationUnit_None, tu.get()
	);
	if (!*tu) {
		printDiagnostics(*tu);
	}
	this->index = std::move(index);
	this->tu = std::move(tu);



}
