//#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>

#include"AIWork/AIWorkFlow.h"
#include"AIWork/Gui/DSLEditor.h"
#include"AIWork/Gui/TheMainWindow.h"
#include"AIWork/Gui/EditorsTabView.h"
#include"AIWork/Gui/FileView.h"
#include<qdebug.h>
#include<iostream>
#include"AIWork/BaseTool.h"
#include <QFile>
#include <QTextStream>
#include <QString>
#include<qfiledialog.h>
#include"qobject.h"
#include"qfileinfo.h"
#include"qset.h"
#include<qmessagebox.h>
#include<qdir.h>
int main(int argc, char* argv[])
{
    //todo 添加一个new scope 操作



    QApplication app(argc, argv);
    QSet<DSLEditor*> loadedFiles;
    QString WorkingFolder;
    EditorsTabView* tabView = new EditorsTabView();
    FileView* fileView = new FileView();
    TheMainWindow* mainWindow = new TheMainWindow();

    auto doLoadFile = [&](const QString& path) {
        // 查找是否已有对应编辑器
        for (auto editor : loadedFiles) {
            if (QFileInfo(editor->getLoadPath()) == QFileInfo(path)) {
                tabView->setCurrent(editor);
                return;
            }
        }
        // 未打开，创建新编辑器
        auto editor = new DSLEditor();
        editor->loadFromFile(path);
        tabView->addTab(std::unique_ptr<QWidget>(editor), QDir(WorkingFolder).relativeFilePath(path));
        loadedFiles.insert(editor);
        tabView->setCurrent(editor);
        QObject::connect(editor, &QWidget::destroyed, [&loadedFiles](QObject* obj) {
            loadedFiles.remove((DSLEditor*)(obj));
            return;
        });
    };



    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, std::unique_ptr<QWidget>(fileView));
    //mainWindow->addDockWidget(Qt::RightDockWidgetArea, std::unique_ptr<QWidget>(tabView));
    mainWindow->setCentralWidget(tabView);
    mainWindow->addMenuAction({ "File","openFolder" }, [&]() {
        QString dir = QFileDialog::getExistingDirectory(
            mainWindow,                 
            "请选择一个文件夹",
            "",                 
            QFileDialog::ShowDirsOnly 
        );
        if (!dir.isEmpty()) {
            WorkingFolder = dir;
            fileView->setRootFolder(dir);
        }
        
    });
    mainWindow->addMenuAction({ "File","save" }, [&]() {
        if (loadedFiles.contains(dynamic_cast<DSLEditor*>(tabView->getCurrent()))) {
            if (dynamic_cast<DSLEditor*>(tabView->getCurrent())->saveBack()) {
            
            }
            else {
                QMessageBox::warning(
                    nullptr, "操作失败","操作失败"
                );
            }
        }
    });
    mainWindow->addMenuAction({ "Code","run" }, [&]() {
        if (loadedFiles.isEmpty()) {
            QMessageBox::warning(nullptr, "", "没有打开任何文件");

        }
        if (loadedFiles.contains(dynamic_cast<DSLEditor*>(tabView->getCurrent()))) {
            auto path = dynamic_cast<DSLEditor*>(tabView->getCurrent())->getLoadPath();

            awf::AIWorkFlow workFlow(QFileInfo(path).absolutePath());
            QString name = "result_" + QFileInfo(path).fileName();
            workFlow.launch(path, "result_"+ QFileInfo(path).fileName());
            if (workFlow.ec.hasErr()) {
                workFlow.ec.printAll();
                QMessageBox::warning(nullptr, "出现错误", workFlow.ec.toString());
            }
            else
            {
                doLoadFile(workFlow.getAbsPath(name));
            }

                
        }
        
    });
    QObject::connect(fileView, &FileView::fileDoubleClicked, doLoadFile);
    mainWindow->show();
    return app.exec();
}