#pragma once

#include <QWidget>
#include "ui_FileView.h"

class QFileSystemModel;
class QSortFilterProxyModel;

class FileView : public QWidget {
    Q_OBJECT

public:
    FileView(QWidget* parent = nullptr);
    ~FileView();

    void setRootFolder(const QString& path);

signals:
    // 双击文件时发出的信号，参数为文件的绝对路径
    void fileDoubleClicked(const QString& filePath);

private slots:
    void onTreeViewDoubleClicked(const QModelIndex& index);

private:
    Ui::FileViewClass ui;
    QFileSystemModel* m_fileSystemModel;      // 源模型
    QSortFilterProxyModel* m_proxyModel;      // 排序代理模型
};