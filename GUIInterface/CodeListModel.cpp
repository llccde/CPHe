#include "CodeListModel.h"

Q_INVOKABLE QModelIndex CodeListModel::index(int row, int column, const QModelIndex& parent) const
{
	return createIndex(row,0);
}

Q_INVOKABLE QModelIndex CodeListModel::parent(const QModelIndex& child) const
{
	return Q_INVOKABLE QModelIndex();
}

Q_INVOKABLE int CodeListModel::rowCount(const QModelIndex& parent) const
{
	if (!codeReader)return 0;
	return codeReader->getRowCount();
}

Q_INVOKABLE int CodeListModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

Q_INVOKABLE QVariant CodeListModel::data(const QModelIndex& index, int role) const
{
	if (!codeReader) return"";

	switch (role)
	{
	case Qt::DisplayRole:
		return codeReader->readLine(index.row());
	default:
		break;
	}
	return QVariant();
	
}
