#include "LanguageBackend.h"
#include "libClangContext.h"
#include "NameViewTreeModel.h"
#include"CodeListModel.h"
CppLanguageBackend::CppLanguageBackend(QStringList& loadedFiles):LanguageBackend(loadedFiles)
{
	codeModel.reset(new CodeListModel());
	treeView.reset(new NameViewTreeModel(nameMapResPack));
	clangContext.reset(new LibClangContext());
}

void CppLanguageBackend::updateModelToFile(int index)
{
}

void CppLanguageBackend::selectedSymbolChanged(const QModelIndex&)
{

}

CppLanguageBackend::~CppLanguageBackend()
{
}
