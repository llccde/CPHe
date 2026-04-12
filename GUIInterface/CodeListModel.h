#pragma once
#include<qabstractitemmodel.h>
#include"CppCodeFileReader.h"
#include"helpfulTypes.h"
class CodeListModel:public QAbstractItemModel{
	std::unique_ptr<CodeFileReader> codeReader;
	QStringList codeEachRow;
public:
	// 通过 QAbstractItemModel 继承
	void reset(std::unique_ptr<CodeFileReader>&& r) {
		codeReader = std::move(r);
		beginResetModel();
		endResetModel();
	}
	Q_INVOKABLE QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	Q_INVOKABLE QModelIndex parent(const QModelIndex& child) const override;
	Q_INVOKABLE int rowCount(const QModelIndex& parent) const override;
	Q_INVOKABLE int columnCount(const QModelIndex& parent) const override;
	Q_INVOKABLE QVariant data(const QModelIndex& index, int role) const override;
};
