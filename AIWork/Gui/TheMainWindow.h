#pragma once

#include <QMainWindow>
#include <QVector>
#include <functional>
#include <memory>
#include "ui_TheMainWindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TheMainWindowClass; };
QT_END_NAMESPACE

class TheMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit TheMainWindow(QWidget* parent = nullptr);
    ~TheMainWindow();

    // 便捷停靠接口：方位 + unique_ptr 管理的 widget
    void addDockWidget(Qt::DockWidgetArea area, std::unique_ptr<QWidget> widget);

    // 菜单添加接口：菜单路径（每级菜单名） + 触发的回调
    // 例如：{"File", "Recent", "Clear List"} 会在 File > Recent 下添加 "Clear List" 动作
    void addMenuAction(const QVector<QString>& path, std::function<void()> callback);

private:
    Ui::TheMainWindowClass* ui;
};