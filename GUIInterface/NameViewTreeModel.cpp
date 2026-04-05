// NameViewTreeModel.cpp
#include"NameViewTreeModel.h"
#include <QBrush>
#include <QFont>

NameViewTreeModel::NameViewTreeModel(SignalUniquePtr<NameMapNode>& rootNode, QObject* parent)
    : QAbstractItemModel(parent)
    , rootNode(rootNode)
{
}

void NameViewTreeModel::refresh()
{
    beginResetModel();
    endResetModel();
}

QModelIndex NameViewTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!rootNode || row < 0 || column != 0)
        return QModelIndex();

    NameMapNode* parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return QModelIndex();

    if (row < parentNode->childrenNum())
    {
        NameMapNode* childNode = (*parentNode)[row];
        if (childNode)
            return createIndex(row, column, childNode);
    }
    return QModelIndex();
}

QModelIndex NameViewTreeModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    NameMapNode* childNode = static_cast<NameMapNode*>(child.internalPointer());
    if (!childNode)
        return QModelIndex();

    NameMapNode* parentNode = childNode->getParent();  // 注意：根节点的 parent 应为 nullptr
    if (!parentNode || rootNode == parentNode)         // 根节点的父级返回无效索引
        return QModelIndex();

    // 找到 parentNode 在其父节点中的行号
    int row = rowOfNode(parentNode);
    return createIndex(row, 0, parentNode);
}

int NameViewTreeModel::rowCount(const QModelIndex& parent) const
{
    if (!rootNode)
        return 0;

    if (!parent.isValid())
        return rootNode->childrenNum();   // 顶层项数量

    NameMapNode* parentNode = nodeFromIndex(parent);
    return parentNode ? parentNode->childrenNum() : 0;
}

int NameViewTreeModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 1;   // 只显示名称一列
}

QVariant NameViewTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    NameMapNode* node = nodeFromIndex(index);
    if (!node)
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
        return node->getMyName();

    case Qt::ForegroundRole:
        if (!node->available())
            return QBrush(Qt::gray);
        break;

    case Qt::FontRole:
        if (node->state == NameMapNode::root)
        {
            QFont font;
            font.setBold(true);
            return font;
        }
        break;
    }
    return QVariant();
}

Qt::ItemFlags NameViewTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant NameViewTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
        return QStringLiteral("Name");
    return QVariant();
}

NameMapNode* NameViewTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return rootNode.get();          // 无效索引视为根节点（用于 rowCount 等）
    return static_cast<NameMapNode*>(index.internalPointer());
}

int NameViewTreeModel::rowOfNode(NameMapNode* node) const
{
    if (!node)
        return -1;

    NameMapNode* parent = node->getParent();
    if (!parent)
        return -1;   // 根节点没有行号

    for (int i = 0; i < parent->childrenNum(); ++i)
    {
        if ((*parent)[i] == node)
            return i;
    }
    return -1;
}