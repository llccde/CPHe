#include"ManyCppOperators.h"
#include"CppContext.h"
#include"Intent.h"
#include"CppContext.h"
#include"libClangContext.h"
#include"QFileReader.h"
#include"CodeAnalyzer.h"
#include"CppCodeVisitor.h"
#include"NameTree.h"
void SetCppMainFile::handleIntent(QVector<Intent*>intent, WorkFlowContext*)
{
	for (auto&i:cppContextManager->getCppContext()->loadedFiles)
	{
		if (i == intent[0]->getData()) {
			cppContextManager->getCppContext()->mainFile = i;
			return;
		}

	}
}
void SetCppMainFile::BeRememberAsIntent(Intent*, WorkFlowContext*)
{
}
void RunCppAnalyze::handleIntent(QVector<Intent*>, WorkFlowContext*)
{
	auto& clangContext = getCppContext()->clangContext;
	clangContext->clear();
	bool first = true;
	for (auto& i : getCppContext()->loadedFiles) {
		auto file = new QFileReader(i);
		bool ismain(file->getFullPath() == getCppContext()->mainFile);
		clangContext->addFile(UniqueFilePtr(file), ismain ? LibClangContext::isMainFile : LibClangContext::notMainFile);
		first = false;
	}
	CodeAnalyzer ana(clangContext.get());
	CPPCodeVisitor cv;
	ana.launch(&cv);
	auto resPack = cv.getNameMap();
	getCppContext()->nameMapResPack = std::move(resPack);
}
void RunCppAnalyze::BeRememberAsIntent(Intent*, WorkFlowContext*)
{
}

RunCppAnalyze::~RunCppAnalyze()
{
}
