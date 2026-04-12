#pragma once
#include"memory.h"
#include"qstringlist.h"
#include<qabstractitemmodel.h>
#include"SignalUniquePtr.h"
class NameMapResPack;
class CodeListModel;
class QAbstractItemModel;
class NameViewTreeModel;
class LanguageBackend {
protected:
	QStringList& loadedFiles;

public:

	virtual void example() {};
	LanguageBackend(QStringList& loadedFiles):loadedFiles(loadedFiles) {
	
	}
	virtual CodeListModel* getCodeListModel() = 0;
	virtual NameViewTreeModel* getSymbolsModel() = 0;
	virtual void selectedSymbolChanged(const QModelIndex&) = 0;
	virtual void updateModelToFile(int index) = 0;

	virtual ~LanguageBackend() {};
};

class LibClangContext;
class CppLanguageBackend:public LanguageBackend {
	std::unique_ptr<CodeListModel> codeModel;
	std::unique_ptr<NameViewTreeModel> treeView;
	std::unique_ptr<LibClangContext> clangContext;
	SignalUniquePtr<NameMapResPack> nameMapResPack;
public:
	
	CppLanguageBackend(QStringList& loadedFiles);
	// 通过 LanguageBackend 继承
	CodeListModel* getCodeListModel() override { return codeModel.get(); };

	NameViewTreeModel* getSymbolsModel() override { return treeView.get(); };

	void updateModelToFile(int index) override;
	// 通过 LanguageBackend 继承
	void selectedSymbolChanged(const QModelIndex&) override;
	~CppLanguageBackend();
};