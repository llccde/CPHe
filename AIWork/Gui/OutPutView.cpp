#include "OutputView.h"
#include "IOController.h"
#include <QTabWidget>
#include <QTabBar>
#include <QMessageBox>

OutputView::OutputView(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::OutPutViewClass)
{
    ui->setupUi(this);

    // 配置标签页可关闭
    ui->tabWidget->setTabsClosable(true);
    ui->tabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectLeftTab);

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
    int id = m_nextId++;

    auto* page = new IOController(this);
    m_pages[id] = page;
    m_pageToId[page] = id;

    // 添加到标签页
    ui->tabWidget->addTab(page, name);

    // 转发输入完成信号（携带 ID）
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

    IOController* page = m_pages[id];
    m_finishedIds.insert(id);

    // 禁用输入
    page->setFinished(true);

    // 修改标签标题（动态获取当前索引）
    int index = ui->tabWidget->indexOf(page);
    if (index != -1) {
        QString title = ui->tabWidget->tabText(index);
        if (!title.endsWith(" ✓"))
            ui->tabWidget->setTabText(index, title + " ✓");
    }
}

void OutputView::output(int id, const QString& text)
{
    if (m_pages.contains(id)) {
        m_pages[id]->output(text);
    }
}

void OutputView::onTabCloseRequested(int index)
{
    QWidget* widget = ui->tabWidget->widget(index);
    IOController* page = qobject_cast<IOController*>(widget);
    if (!page || !m_pageToId.contains(page))
        return;

    int id = m_pageToId[page];

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
            return;
    }

    // 关闭页面
    ui->tabWidget->removeTab(index);
    m_pages.remove(id);
    m_pageToId.remove(page);
    m_finishedIds.remove(id);
    page->deleteLater();

    emit close(id);
}

void OutputView::onInputDone(const QString& text)
{
    // 备用，可通过 lambda 扩展
}