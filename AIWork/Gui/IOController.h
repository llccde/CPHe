#pragma once

#include <QWidget>
#include "ui_IOController.h"

class QStringListModel;

QT_BEGIN_NAMESPACE
namespace Ui { class IOControllerClass; };
QT_END_NAMESPACE

class IOController : public QWidget {
    Q_OBJECT

public:
    IOController(QWidget* parent = nullptr);
    ~IOController();
    void output(const QString& text);
    void setFinished(bool finished);
signals:
    void inputDone(const QString& text);

private slots:
    void onFinishInput();
    void onHistoryItemClicked(const QModelIndex& index);
    void onBackCurrent();

private:
    void setEditMode(bool editing);

    Ui::IOControllerClass* ui;
    QStringListModel* m_historyModel;
    QString m_lastEditingText;
    bool m_isViewingHistory = false;
};