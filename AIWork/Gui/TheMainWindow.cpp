#include "TheMainWindow.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

// 辅助函数：从 QMenuBar 中查找或创建顶层菜单
static QMenu* findOrCreateMenu(QMenuBar* mb, const QString& title) {
    for (QAction* action : mb->actions()) {
        if (action->text() == title) {
            QMenu* menu = action->menu();
            if (menu) return menu;
        }
    }
    return mb->addMenu(title);
}

// 辅助函数：从 QMenu 中查找或创建子菜单
static QMenu* findOrCreateMenu(QMenu* menu, const QString& title) {
    for (QAction* action : menu->actions()) {
        if (action->text() == title) {
            QMenu* subMenu = action->menu();
            if (subMenu) return subMenu;
        }
    }
    return menu->addMenu(title);
}

TheMainWindow::TheMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::TheMainWindowClass)
{
    ui->setupUi(this);
}

TheMainWindow::~TheMainWindow()
{
    delete ui;
}

void TheMainWindow::addDockWidget(Qt::DockWidgetArea area,
    std::unique_ptr<QWidget> widget)
{
    if (!widget)
        return;

    auto dock = std::make_unique<QDockWidget>(this);
    dock->setWidget(widget.release());
    dock->setWindowTitle(dock->widget()->windowTitle());
    QMainWindow::addDockWidget(area, dock.release());
}

void TheMainWindow::addMenuAction(const QVector<QString>& path,
    std::function<void()> callback)
{
    if (path.isEmpty())
        return;

    QMenuBar* mb = menuBar();                // 主窗口的菜单栏（若不存在则自动创建）
    QMenu* currentMenu = nullptr;            // 当前深度的菜单对象
    const int lastIdx = path.size() - 1;     // 最后一级是动作

    for (int i = 0; i < path.size(); ++i) {
        const QString& text = path.at(i);

        if (i == lastIdx) {
            // 最后一级：创建动作
            QAction* action = nullptr;
            if (currentMenu) {
                action = currentMenu->addAction(text);
            }
            else {
                action = mb->addAction(text);   // 直接挂在菜单栏上的动作
            }

            if (callback)
                QObject::connect(action, &QAction::triggered, callback);
        }
        else {
            // 中间级菜单：查找或创建
            if (i == 0) {
                currentMenu = findOrCreateMenu(mb, text);
            }
            else {
                currentMenu = findOrCreateMenu(currentMenu, text);
            }
        }
    }
}