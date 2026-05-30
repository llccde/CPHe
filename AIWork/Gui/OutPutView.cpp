#include "OutputView.h"
#include "IOController.h"
#include <QTabWidget>
#include <QTabBar>
#include<qmessagebox.h>
OutputView::OutputView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OutPutViewClass)
{
    ui->setupUi(this);

    // 配置标签页可关闭
    ui->tabWidget->setTabsClosable(true);
    ui->tabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab); // 关闭后选择左侧标签

    // 移除 UI 文件中的占位标签页
    if (ui->tabWidget->count() > 0) {
        ui->tabWidget->removeTab(0);
    }
    ui->tabWidget->setTabPosition(QTabWidget::West);

    // 连接标签页关闭请求信号
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested,
        this, &OutputView::onTabCloseRequested);
}

OutputView::~OutputView()
{
    delete ui;
}

int OutputView::launch(const QString& name)
{
    // 分配新 ID
    int id = m_nextId++;

    // 创建 IOController 页面
    auto* page = new IOController(this);
    m_pages[id] = page;

    // 添加到标签页
    int index = ui->tabWidget->addTab(page, name);
    m_idToIndex[id] = index;
    m_indexToId[index] = id;

    // 转发该页面的输入完成信号（带 ID）
    connect(page, &IOController::inputDone,
        this, [this, id](const QString& text) {
            emit inputDone(id, text);
        });

    // 默认切换到新页面
    ui->tabWidget->setCurrentWidget(page);

    return id;
}

void OutputView::finish(int id)
{
    if (!m_pages.contains(id))
        return;

    // 标记为已完成
    m_finishedIds.insert(id);

    // 禁用该页面的输入
    m_pages[id]->setFinished(true);

    // 可选：修改标签标题以反映状态
    int index = m_idToIndex[id];
    QString title = ui->tabWidget->tabText(index);
    if (!title.endsWith(" ✓"))
        ui->tabWidget->setTabText(index, title + " ✓");
}

void OutputView::output(int id, const QString& text)
{
    if (m_pages.contains(id)) {
        m_pages[id]->output(text);
    }
}

#include <QMessageBox>  // 添加到文件头部

void OutputView::onTabCloseRequested(int index)
{
    if (!m_indexToId.contains(index))
        return;

    int id = m_indexToId[index];

    // 如果页面未完成，弹出确认对话框
    if (!m_finishedIds.contains(id)) {
        QString pageName = ui->tabWidget->tabText(index);
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "确认关闭",
            QString("页面 \"%1\" 尚未完成，确定要关闭吗？").arg(pageName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (reply == QMessageBox::No)
            return;   // 用户取消关闭
    }

    // 关闭页面（已完成直接关闭，未完成且用户确认后也走这里）
    ui->tabWidget->removeTab(index);

    IOController* page = m_pages.take(id);
    page->deleteLater();

    m_finishedIds.remove(id);
    m_idToIndex.remove(id);
    m_indexToId.remove(index);

    emit close(id);
}

void OutputView::onInputDone(const QString& text)
{
    // （备用，直接通过 lambda 连接了，此处可保留以便扩展）
}