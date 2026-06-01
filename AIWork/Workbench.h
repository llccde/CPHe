#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <functional>

class DSLEditor;
class EditorsTabView;
class FileView;
class TheMainWindow;
class OutputView;

class Workbench : public QObject {
    Q_OBJECT
public:
    explicit Workbench(QApplication& app, QObject* parent = nullptr);
    ~Workbench();

private:
    // ---------- 原 main 中的顶层 lambda，现在变成成员方法 ----------
    void doLoadFile(const QString& path);
    void doLaunch(QString path, QString folder, std::function<void()> onFinish);
    void onAboutToQuit();
    void onOpenFolder();
    void onSave();
    void onRun();

    // ---------- 原 main 中的变量，都变成成员 ----------
    QSettings settings;
    QString lastFolderPath;
    QStringList lastOpenedFiles;

    QSet<DSLEditor*> loadedFiles;
    QSet<QString> fileRunning;
    QString workingFolder;

    // 这些 UI 组件的生命周期由 Workbench 管理
    EditorsTabView* tabView = nullptr;
    FileView* fileView = nullptr;
    TheMainWindow* mainWindow = nullptr;
    OutputView* outputView = nullptr;

    QApplication& app;   // 用于连接 aboutToQuit 等
};