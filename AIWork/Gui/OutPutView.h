#pragma once

#include <QWidget>
#include <QMap>
#include "ui_OutPutView.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OutPutViewClass; };
QT_END_NAMESPACE

class IOController;

class OutputView : public QWidget {
    Q_OBJECT

public:
    explicit OutputView(QWidget* parent = nullptr);
    ~OutputView();

    // 创建新页面，返回页面 ID
    int launch(const QString& name);

    // 标记页面为完成，允许关闭
    void finish(int id);

    // 向指定 ID 的页面输出文本
    void output(int id, const QString& text);

signals:
    // 用户输入完成信号（携带页面 ID 和输入文本）
    void inputDone(int id, const QString& text);

    // 页面被关闭信号（只有已完成页面才会触发）
    void close(int id);

private slots:
    void onTabCloseRequested(int index);
    void onInputDone(const QString& text);

private:
    Ui::OutPutViewClass* ui;
    QMap<int, IOController*> m_pages;       // id → 页面对象
    QMap<int, int> m_idToIndex;             // id → tab索引
    QMap<int, int> m_indexToId;             // tab索引 → id
    QSet<int> m_finishedIds;                // 已完成页面的 ID 集合
    int m_nextId = 0;                       // 下一个可用 ID
};