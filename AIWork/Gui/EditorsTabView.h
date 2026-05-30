#pragma once

#include <QWidget>
#include <memory>
#include "ui_EditorsTabView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class EditorsTabViewClass; };
QT_END_NAMESPACE

class EditorsTabView : public QWidget {
    Q_OBJECT

public:
    explicit EditorsTabView(QWidget* parent = nullptr);
    ~EditorsTabView();

    // 添加标签页，接收 unique_ptr 所有权
    void addTab(std::unique_ptr<QWidget> widget, const QString& label = QString());
    QWidget* getCurrent();
    void setCurrent(QWidget*);
    void setTabTitle(QWidget*, QString title);
private slots:
    void onTabCloseRequested(int index);

private:
    Ui::EditorsTabViewClass* ui;
};