// NameViewTreeModel.h
#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include"NameTree.h"  // 包含 NameMapNode 的定义
#include"SignalUniquePtr.h"
class NameViewTreeModel : public QAbstractItemModel {
    Q_OBJECT
private:
    SignalUniquePtr<NameMapNode>& rootNode;
public:
    explicit NameViewTreeModel(SignalUniquePtr<NameMapNode>& root, QObject* parent = nullptr);
    ~NameViewTreeModel() override = default;

    // 强制重新加载整个模型（当外部修改了节点结构时调用）
    void refresh();

    // QAbstractItemModel 接口实现
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    NameMapNode* nodeFromIndex(const QModelIndex& index) const;
    int rowOfNode(NameMapNode* node) const;
};