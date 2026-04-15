#include "LanguageBackend.h"
#include "libClangContext.h"
#include "NameViewTreeModel.h"
#include"CodeListModel.h"
#include"QFileReader.h"
#include"CodeAnalyzer.h"
#include"CppCodeVisitor.h"
#include"qmessagebox.h"
#include"qguiapplication.h"
#include"qclipboard.h"
#include"qevent.h"
namespace {
	class CloseOnClickFilter : public QObject {
	public:
		bool eventFilter(QObject* watched, QEvent* event) override {
			if (event->type() == QEvent::MouseButtonPress) {
				QMessageBox* msgBox = qobject_cast<QMessageBox*>(watched);
				if (msgBox) {
					msgBox->accept();  // 相当于点击了“确定”/关闭
					return true;      // 事件已处理，不再传递
				}
			}
			return QObject::eventFilter(watched, event);
		}
	};
}

CppLanguageBackend::CppLanguageBackend(QStringList& loadedFiles):LanguageBackend(loadedFiles)
{
	codeModel.reset(new CodeListModel());
	treeViewModel.reset(new NameViewTreeModel(nameMapResPack));
	clangContext.reset(new LibClangContext());
}

void CppLanguageBackend::updateModelToFile(int index)
{
	CloseOnClickFilter fileter;
	if (index == -1) {
		QMessageBox msg(QMessageBox::Icon(), QString("warning"), QString("please select a .cpp or .h file"));
		msg.installEventFilter(&fileter);
		msg.exec();
		return;
	}
	QString& currentMain = loadedFiles[index];
	clangContext->clear();
	bool first = true;
	for (auto& i : loadedFiles) {
		auto file = new QFileReader(i);
		bool ismain(file->getFullPath() == currentMain);
		clangContext->addFile(UniqueFilePtr(file), ismain ? LibClangContext::isMainFile : LibClangContext::notMainFile);
		first = false;
	}
	CodeAnalyzer ana(clangContext.get());
	CPPCodeVisitor cv;
	ana.launch(&cv);
	auto resPack = cv.getNameMap();
	nameMapResPack.resetAsync(std::move(resPack));
}

void CppLanguageBackend::selectedSymbolChanged(const QModelIndex& index)
{
	auto raw = this->treeViewModel->getRawNode(index);
	auto pos = raw->getPosition();
	std::unique_ptr<CppCodeFileReader> reader(new CppCodeFileReader(true, pos, 4));
	codeModel->reset(std::move(reader));
}

CppLanguageBackend::~CppLanguageBackend()
{
}

void CppLanguageBackend::copyCode(const QModelIndex& index)
{
	auto raw = this->treeViewModel->getRawNode(index);
	auto pos = raw->getPosition();
	std::unique_ptr<CppCodeFileReader> reader(new CppCodeFileReader(true, pos, 4));
	QGuiApplication::clipboard()->setText(reader->readAll());
}
