#include "NameMapView.h"
#include"NameViewTreeModel.h"
#include"NameTree.h"
#include"libClangContext.h"
#include"qfiledialog.h"
#include"QFileReader.h"
#include <QSettings>
#include<qfileinfo.h>
#include<QStandardPaths>
#include <QStandardPaths>
#include"CppCodeVisitor.h"
#include"CodeAnalyzer.h"
#include"HighlightDelegate.h"
#include"qmenu.h"
#include"qlistview.h"

static QString openExplore() {
	QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	QDir().mkpath(configPath);
	QSettings settings(configPath + "/config.ini", QSettings::IniFormat);

	QString lastDir = settings.value("LastDirectory", QDir::homePath()).toString();

	if (!QDir(lastDir).exists())
		lastDir = QDir::homePath();

	QString filePath = QFileDialog::getOpenFileName(nullptr, "", lastDir, "(*.cpp *.h)");
	if (!filePath.isEmpty()) {
		QFileInfo info(filePath);
		settings.setValue("LastDirectory", info.absolutePath());
		settings.sync();
	}
	return filePath;
}
NameMapView::NameMapView(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::NameMapViewClass())
{
	backend.reset(new CppLanguageBackend(loadedFile));
	ui->setupUi(this);

	loadedFileDelegate.reset(new HighlightDelegateOfQString());
	ui->nameMapView->setModel(backend->getSymbolsModel());
	ui->loadedFiles->setModel(&loadedFileModel);
	ui->loadedFiles->setItemDelegate(loadedFileDelegate.get());
	ui->codeView->setModel(backend->getCodeListModel());
	connect(ui->refreshButton, &QPushButton::clicked, this, [this]() {
		backend->updateModelToFile(currentSelectLoadedFile);
	});
	connect(ui->loadButton, &QPushButton::clicked, this, [this]() {
		QString filePath = openExplore();
		if (!filePath.isEmpty()) {
			if (!loadedFile.contains(filePath)) {
				loadedFile.append(std::move(filePath));
			}
		}
		loadedFileModel.setStringList(loadedFile);
	});
	connect(ui->loadedFiles, &QAbstractItemView::clicked, this, [this](const QModelIndex& index) {
		if (index.isValid()) {
			this->setCurrentSelectLoadedFile(index.row());
		}
		else {
			this->setCurrentSelectLoadedFile(-1);
		}
		
	});
	connect(this, &NameMapView::currentSelectLoadedFileChanged, this, [this](int index) {
		this->ui->removeLoadedFileButton->setEnabled(index != -1);

		
	});
	connect(ui->removeLoadedFileButton, &QPushButton::clicked, this, [this]() {
		int n = getCurrentSelectLoadedFile();
		if (n == -1)return;
		loadedFile.remove(n);
		loadedFileModel.setStringList(loadedFile);
	});
	connect(ui->setMainFileButton, &QPushButton::clicked, this, [this]() {
		this->currentMainFile = loadedFile[getCurrentSelectLoadedFile()];
		this->loadedFileDelegate->clearAllHighlightStrings();
		this->loadedFileDelegate->addHighlightString(currentMainFile);
		this->ui->loadedFiles->update();
	});
	connect(&loadedFileModel, &QStringListModel::dataChanged, this, [this]() {
		this->setCurrentSelectLoadedFile(-1);
	});
	connect(ui->nameMapView, &QTreeView::clicked, this, [this](const QModelIndex& index) {
		backend->selectedSymbolChanged(index);
		
	});
	connect(ui->nameMapView, &QTreeView::clicked, this, [this](const QModelIndex& index) {
		backend->selectedSymbolChanged(index);

	});
	ui->nameMapView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->nameMapView, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
		QMenu menu;
		auto index = this->ui->nameMapView->indexAt(pos);
		menu.addAction(QString("copy"), [this,&index]() {
			this->backend->copyCode(index);
		});
		menu.exec(QCursor::pos());
	});
	//connect(ui->loadedFiles,&QListView)
	
}

NameMapView::~NameMapView()
{
	delete ui;
}


int NameMapView::getCurrentSelectLoadedFile()
{
	//assert(currentSelectLoadedFile >= 0 && currentSelectLoadedFile < loadedFile.size());
	return currentSelectLoadedFile;
}

void NameMapView::setCurrentSelectLoadedFile(int val)
{
	currentSelectLoadedFile = val;
	emit currentSelectLoadedFileChanged(val);
}

