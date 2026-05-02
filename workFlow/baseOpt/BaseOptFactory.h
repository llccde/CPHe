#pragma once
#include"TaskFactory.h"
#include<qmetatype.h>
#include"qobject.h"
#include <QMetaEnum>
class BaseOptFactory :public QObject,public TaskFactory{
	Q_OBJECT
public:
	// 通过 TaskFactory 继承
	enum BaseTasks {
		newScope
	};
	Q_ENUM(BaseTasks);
	void initSubContext() override;
	
	std::unique_ptr<BaseOperator> getOperator(const QString& name) override;
	const QString myName() override;
	std::vector<QString> getOperators() override;
};