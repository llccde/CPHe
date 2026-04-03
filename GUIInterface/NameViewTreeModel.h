#pragma once
#include<qabstractitemmodel.h>
#include "CodeAnalyzer.h"
#include"NameTree.h"
class NameViewTreeModel :public QAbstractItemModel {
	Q_OBJECT
	using NameMapNode = NameMap;
	NameMapNode& nameMap;
public:

	Q_INVOKABLE QModelIndex index(int row, int column, const QModelIndex& parent) const override;

	Q_INVOKABLE QModelIndex parent(const QModelIndex& child) const override;

	Q_INVOKABLE int rowCount(const QModelIndex& parent) const override;

	Q_INVOKABLE int columnCount(const QModelIndex& parent) const override;

	Q_INVOKABLE QVariant data(const QModelIndex& index, int role) const override;

	NameMapNode* nodeFromIndex(QModelIndex& index) {
		if (index.isValid()) {
			return static_cast<NameMapNode*>(index.internalPointer());
		}
	}
};