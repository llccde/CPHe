#include "IOController.h"
#include <QStringListModel>
#include <QListView>

IOController::IOController(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::IOControllerClass)
    , m_historyModel(new QStringListModel(this))
    , m_isViewingHistory(false)
{
    ui->setupUi(this);

    // 设置输出区域为只读（仅用于展示）
    ui->output->setReadOnly(true);

    // 为历史列表设置模型
    ui->inputHistory->setModel(m_historyModel);
    // 设置列表的选择行为
    ui->inputHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 初始状态：编辑模式
    ui->backCurrentInput->setEnabled(false);
    ui->finishCurrentInput->setEnabled(true);
    ui->currentInputEditor->setReadOnly(false);

    // 连接信号槽
    connect(ui->finishCurrentInput, &QPushButton::clicked,
        this, &IOController::onFinishInput);
    connect(ui->backCurrentInput, &QPushButton::clicked,
        this, &IOController::onBackCurrent);
    connect(ui->inputHistory, &QListView::clicked,
        this, &IOController::onHistoryItemClicked);
}

IOController::~IOController()
{
    delete ui;
}

void IOController::output(const QString& text)
{
    // 外部直接输出文本到输出区
    ui->output->append(text);
}

void IOController::onFinishInput()
{
    const QString text = ui->currentInputEditor->toPlainText().trimmed();
    if (text.isEmpty())
        return; // 忽略空输入

    // 添加到输出区末尾
    ui->output->append(text);

    // 添加到历史记录模型
    m_historyModel->insertRow(m_historyModel->rowCount());
    QModelIndex idx = m_historyModel->index(m_historyModel->rowCount() - 1);
    m_historyModel->setData(idx, text);

    // 发射输入完成信号
    emit inputDone(text);

    // 清空当前输入编辑器，保持编辑模式
    ui->currentInputEditor->clear();
    m_lastEditingText.clear();

    // 确保界面处于编辑状态
    setEditMode(true);

    // 将焦点还给输入框
    ui->currentInputEditor->setFocus();
}

void IOController::onHistoryItemClicked(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    // 保存当前正在编辑的内容（草稿）
    if (!m_isViewingHistory) {
        m_lastEditingText = ui->currentInputEditor->toPlainText();
    }

    // 切换到查看历史模式
    setEditMode(false);

    // 显示选中的历史条目内容，并设为只读
    QString historyText = m_historyModel->data(index, Qt::DisplayRole).toString();
    ui->currentInputEditor->setPlainText(historyText);
}
// IOController.cpp 实现
void IOController::setFinished(bool finished)
{
    if (finished) {
        // 已完成：输入变为只读，完成按钮无效，返回按钮无效
        ui->currentInputEditor->setReadOnly(true);
        ui->finishCurrentInput->setEnabled(false);
        ui->backCurrentInput->setEnabled(false);
        ui->inputHistory->setEnabled(false);   // 同时禁止查看历史
    }
    else {
        ui->currentInputEditor->setReadOnly(false);
        ui->finishCurrentInput->setEnabled(true);
        ui->backCurrentInput->setEnabled(false);   // 非查看模式
        ui->inputHistory->setEnabled(true);
    }
}
void IOController::onBackCurrent()
{
    // 切换回编辑模式，恢复之前保存的草稿
    if (m_isViewingHistory) {
        setEditMode(true);
        ui->currentInputEditor->setPlainText(m_lastEditingText);
        // 清除历史列表的选中状态（可选，避免视觉混淆）
        ui->inputHistory->clearSelection();
    }
}

void IOController::setEditMode(bool editing)
{
    m_isViewingHistory = !editing;
    ui->currentInputEditor->setReadOnly(!editing);
    ui->finishCurrentInput->setEnabled(editing);
    ui->backCurrentInput->setEnabled(!editing);
}