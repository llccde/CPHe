#include "NameViewTreeModel.h"


Q_INVOKABLE QModelIndex NameViewTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	return Q_INVOKABLE QModelIndex();
}

Q_INVOKABLE QModelIndex NameViewTreeModel::parent(const QModelIndex& child) const
{
	return Q_INVOKABLE QModelIndex();
}

Q_INVOKABLE int NameViewTreeModel::rowCount(const QModelIndex& parent) const
{
	return Q_INVOKABLE int();
}

Q_INVOKABLE int NameViewTreeModel::columnCount(const QModelIndex& parent) const
{
	return Q_INVOKABLE int();
}

Q_INVOKABLE QVariant NameViewTreeModel::data(const QModelIndex& index, int role) const
{
	return Q_INVOKABLE QVariant();
}
