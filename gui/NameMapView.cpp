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
	ui->setupUi(this);
	nameMap.resetSync(nullptr);
	dataModel.reset(new NameViewTreeModel(nameMap, this));
	clangContext.reset(new LibClangContext());
	loadedFileDelegate.reset(new HighlightDelegateOfQString());
	ui->nameMapView->setModel(dataModel.get());
	ui->loadedFiles->setModel(&loadedFileModel);
	ui->loadedFiles->setItemDelegate(loadedFileDelegate.get());
	connect(ui->refreshButton, &QPushButton::clicked, this, [this]() {
		loadIntoClangContext();
		runCodeAnalyze();
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
		loadedFile.remove(getCurrentSelectLoadedFile());
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
	//connect(ui->loadedFiles,&QListView)
	
}

NameMapView::~NameMapView()
{
	delete ui;
}

void NameMapView::loadIntoClangContext()
{
	clangContext->clear();
	bool first = true;
	for (auto& i : loadedFile) {
		auto file = new QFileReader(i);
		bool ismain(file->getFullPath() == currentMainFile);
		clangContext->addFile(UniqueFilePtr(file), ismain?LibClangContext::isMainFile: LibClangContext::notMainFile);
		first = false;
	}
}

void NameMapView::runCodeAnalyze()
{
	CodeAnalyzer ana(clangContext.get());
	CPPCodeVisitor cv;
	ana.launch(&cv);
	auto resPack = cv.getNameMap();
	nameMap.resetAsync(std::move(resPack));


}

int NameMapView::getCurrentSelectLoadedFile()
{
	assert(currentSelectLoadedFile >= 0 && currentSelectLoadedFile < loadedFile.size());
	return currentSelectLoadedFile;
}

void NameMapView::setCurrentSelectLoadedFile(int val)
{
	currentSelectLoadedFile = val;
	emit currentSelectLoadedFileChanged(val);
}

