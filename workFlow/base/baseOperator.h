#pragma once
#include<assert.h>
#include<qvector.h>
class Intent;
class QWidget;
class WorkFlowContext;
struct NamedField {
	QString name;
	enum FieldType {
		myInt,
		myString,
		myBool
	} type;
	QWidget* getController = nullptr;
};
class BaseOperator {
public:

	//执行
	virtual void handleIntent(QVector<Intent*>, WorkFlowContext*) = 0;
	
	//存储执行结果进上下文
	virtual void BeRememberAsIntent(Intent*, WorkFlowContext*) = 0;

	//intent 的功能应该尽可能简单
	//virtual std::vector<NamedField> getAllParamsController() = 0;
};