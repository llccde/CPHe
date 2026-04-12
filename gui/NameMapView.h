#pragma once
#include"memory.h"
#include <QWidget>
#include "ui_NameMapView.h"
#include<qstringlistmodel.h>
#include"SignalUniquePtr.h"
#include"CodeListModel.h"
QT_BEGIN_NAMESPACE
namespace Ui { class NameMapViewClass; };
QT_END_NAMESPACE
class NameViewTreeModel;
class NameMapNode;
class LibClangContext;
class HighlightDelegateOfQString;
class NameMapResPack;
class NameMapView : public QWidget
{
	Q_OBJECT
private:
	std::unique_ptr<CodeListModel> codeModel;
	std::unique_ptr<NameViewTreeModel> dataModel;
	SignalUniquePtr<NameMapResPack> nameMap;
	std::unique_ptr<LibClangContext> clangContext;
	std::unique_ptr<HighlightDelegateOfQString> loadedFileDelegate;
	QStringListModel loadedFileModel;
	QStringList loadedFile;
	QString currentMainFile = "";
	int currentSelectLoadedFile = -1;
public:
	NameMapView(QWidget *parent = nullptr);
	~NameMapView();
public:
	void loadIntoClangContext();
	void runCodeAnalyze();
	int getCurrentSelectLoadedFile();
	void setCurrentSelectLoadedFile(int val);
private:
	Ui::NameMapViewClass *ui;
signals:
	void currentSelectLoadedFileChanged(int index);
};

