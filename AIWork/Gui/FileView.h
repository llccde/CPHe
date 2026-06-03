#pragma once

#include <QWidget>
#include "ui_FileView.h"
#include<qfileinfo.h>
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
    void onCustomContextMenu(const QPoint& pos);



private:
    // 根据代理索引获取对应的源文件信息（自动映射）
    QFileInfo fileInfoFromIndex(const QModelIndex& proxyIndex) const;
    // 显示简单的错误对话框
    void showError(const QString& message) const;

    Ui::FileViewClass ui;
    QFileSystemModel* m_fileSystemModel;      // 源模型
    QSortFilterProxyModel* m_proxyModel;      // 排序代理模型
};