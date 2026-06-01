#include "EditorsTabView.h"

EditorsTabView::EditorsTabView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::EditorsTabViewClass())
{
    ui->setupUi(this);

    // 移除 UI 文件中默认的两个占位标签页
    while (ui->tabWidget->count() > 0) {
        QWidget* page = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete page;
    }

    // 启用标签页上的关闭按钮（叉号）
    ui->tabWidget->setTabsClosable(true);

    // 连接关闭信号
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested,
        this, &EditorsTabView::onTabCloseRequested);
}

EditorsTabView::~EditorsTabView()
{
    delete ui;
}

void EditorsTabView::addTab(std::unique_ptr<QWidget> widget, const QString& label)
{
    if (!widget)
        return;

    // 释放 unique_ptr 所有权，将控件添加到 QTabWidget
    // QTabWidget 会为控件设置合适的父对象
    ui->tabWidget->addTab(widget.release(), label.isEmpty()
        ? QString("Tab %1").arg(ui->tabWidget->count() + 1)
        : label);
}

QWidget* EditorsTabView::getCurrent()
{
    return ui->tabWidget->currentWidget();
}

void EditorsTabView::setCurrent(QWidget* w)
{
    ui->tabWidget->setCurrentWidget(w);
}

void EditorsTabView::setTabTitle(QWidget*w, QString title)
{
    ui->tabWidget->setTabText(ui->tabWidget->indexOf(w), title);
}

void EditorsTabView::onTabCloseRequested(int index)
{
    QWidget* page = ui->tabWidget->widget(index);
    if (page) {
        // 从 QTabWidget 中移除标签页（不会自动销毁控件）
        ui->tabWidget->removeTab(index);
        // 安全销毁控件
        page->deleteLater();
    }
}