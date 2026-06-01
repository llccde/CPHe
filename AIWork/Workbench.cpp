#include "Workbench.h"
#include "AIWorkFlow.h"
#include "Gui/DSLEditor.h"
#include "Gui/TheMainWindow.h"
#include "Gui/EditorsTabView.h"
#include "Gui/FileView.h"
#include "Gui/OutPutView.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QThread>
#include"LaunchTask.h"
Workbench::Workbench(QApplication& app, QObject* parent)
    : QObject(parent)
    , settings("LLBB", "CPHE")
    , lastFolderPath(settings.value("lastFolederPath").toString())
    , lastOpenedFiles(settings.value("lastOpendFiles").toString().split("\n"))
    , app(app)
{
    // 创建 UI
    tabView = new EditorsTabView();
    fileView = new FileView();
    mainWindow = new TheMainWindow();
    outputView = new OutputView();

    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, std::unique_ptr<QWidget>(fileView));
    mainWindow->setCentralWidget(tabView);
    mainWindow->addDockWidget(Qt::BottomDockWidgetArea, std::unique_ptr<QWidget>(outputView));
    outputView->launch("hello");

    // 连接信号
    QObject::connect(&app, &QApplication::aboutToQuit, this, &Workbench::onAboutToQuit);

    mainWindow->addMenuAction({ "File", "openFolder" }, [this]() { onOpenFolder(); });
    mainWindow->addMenuAction({ "File", "save" }, [this]() { onSave(); })
        ->setShortcut(QKeySequence::Save);
    mainWindow->addMenuAction({ "Code", "run" }, [this]() { onRun(); });

    QObject::connect(fileView, &FileView::fileDoubleClicked,
        this, &Workbench::doLoadFile);

    // 恢复上次工作区
    if (!lastFolderPath.isEmpty() && QDir(lastFolderPath).exists()) {
        workingFolder = lastFolderPath;
        fileView->setRootFolder(workingFolder);
    }
    for (const QString& path : qAsConst(lastOpenedFiles)) {
        if (!path.isEmpty() && QFile::exists(path)) {
            doLoadFile(path);
        }
    }

    mainWindow->show();
}

Workbench::~Workbench()
{
    // mainWindow 会在父对象链中被自动删除，这里仅作占位
}

// ---------- 方法定义（代码与原来 lambda 完全一致） ----------

void Workbench::doLoadFile(const QString& path)
{
    if (!QFile(path).exists()) return;

    for (auto editor : qAsConst(loadedFiles)) {
        if (QFileInfo(editor->getLoadPath()) == QFileInfo(path)) {
            tabView->setCurrent(editor);
            return;
        }
    }

    auto editor = new DSLEditor();
    editor->loadFromFile(path);
    QString title = QDir(workingFolder).relativeFilePath(path);
    tabView->addTab(std::unique_ptr<QWidget>(editor), title);
    loadedFiles.insert(editor);
    tabView->setCurrent(editor);

    // 这些局部 lambda 保持不变，通过捕获成员变量工作
    std::function<void(QObject*)> onEditorDestroyed = [this](QObject* obj) {
        loadedFiles.remove(static_cast<DSLEditor*>(obj));
        };
    QObject::connect(editor, &QWidget::destroyed, onEditorDestroyed);

    std::function<void(DSLEditor*)> onBeModified = [this, title](DSLEditor* ed) {
        tabView->setTabTitle(ed, title + "*");
        };
    QObject::connect(editor, &DSLEditor::beModifyed, onBeModified);

    std::function<void(DSLEditor*)> onSaved = [this, title](DSLEditor* ed) {
        tabView->setTabTitle(ed, title);
        };
    QObject::connect(editor, &DSLEditor::saved, onSaved);
}

void Workbench::doLaunch(QString path, QString folder, std::function<void()> onFinish)
{
    auto AIWF = new awf::AIWorkFlow(folder);
    int id = outputView->launch(QFileInfo(path).fileName());
    QString outFileName = "result_" + QFileInfo(path).fileName();

    std::function<void(const QString&)> onOutPut = [this, id](const QString& data) {
        outputView->output(id, data + "\n");
        };
    QObject::connect(AIWF, &awf::AIWorkFlow::outPut, mainWindow,
        onOutPut, Qt::QueuedConnection);

    QThread* thread = new QThread;
    AIWF->moveToThread(thread);

    std::function<void()> onStarted = [AIWF, thread, path, outFileName]() {
        QString fileName = QFileInfo(path).fileName();
        AIWF->launch(fileName, outFileName);
        if (AIWF->ec.hasErr())
            emit AIWF->outPut(AIWF->ec.toString());
        else
            emit AIWF->outPut("task done");
        thread->quit();
        };
    QObject::connect(thread, &QThread::started, AIWF, onStarted);

    // 注意这里捕获了 this 以便调用成员方法 doLoadFile / doLaunch
    std::function<void()> onFinished = [this, id, folder, outFileName, thread, AIWF, onFinish]() {
        outputView->finish(id);
        doLoadFile(QDir(folder).absoluteFilePath(outFileName));
        AIWF->deleteLater();
        thread->deleteLater();
        if (onFinish) onFinish();
        };
    QObject::connect(thread, &QThread::finished, mainWindow,
        onFinished, Qt::QueuedConnection);

    thread->start();
}

void Workbench::onAboutToQuit()
{
    settings.setValue("lastFolederPath", workingFolder);
    QStringList openFiles;
    for (DSLEditor* editor : qAsConst(loadedFiles)) {
        QString path = editor->getLoadPath();
        if (!path.isEmpty())
            openFiles << path;
    }
    settings.setValue("lastOpendFiles", openFiles.join("\n"));
}

void Workbench::onOpenFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        mainWindow, "请选择一个文件夹", "",
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        workingFolder = dir;
        fileView->setRootFolder(dir);
    }
}

void Workbench::onSave()
{
    auto current = dynamic_cast<DSLEditor*>(tabView->getCurrent());
    if (loadedFiles.contains(current)) {
        if (!current->saveBack()) {
            QMessageBox::warning(nullptr, "操作失败", "操作失败");
        }
    }
}

void Workbench::onRun()
{
    if (loadedFiles.isEmpty()) {
        QMessageBox::warning(nullptr, "", "没有打开任何文件");
        return;
    }
    auto current = dynamic_cast<DSLEditor*>(tabView->getCurrent());
    QString path = current->getLoadPath();
    QString absPath = QFileInfo(path).absoluteFilePath();

    if (fileRunning.contains(absPath)) {
        QMessageBox::warning(nullptr, "操作失败", "该文件正在被执行");
        return;
    }
    fileRunning.insert(absPath);

    // 创建任务
    auto task = new LaunchTask(absPath, QFileInfo(path).absolutePath(), outputView, this);

    // 任务完成后重新加载文件，并从 running 集合移除
    connect(task, &LaunchTask::resultReady, this, [this, absPath](const QString& resultPath) {
        doLoadFile(resultPath);
        fileRunning.remove(absPath);
        });

    // 任务结束时删除 task 自身（finished 信号之后 task 不再被需要）
    connect(task, &LaunchTask::finished, task, &QObject::deleteLater);

    task->start();
}