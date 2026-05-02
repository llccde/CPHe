#pragma once
#include"memory.h"
#include"qstring.h"
class WorkFlowContext;
class BaseOperator;
class TaskFactory {
protected:
	WorkFlowContext* mainContext;
	virtual void initSubContext() = 0;
public:
	void setMainContext(WorkFlowContext* context) {
		mainContext = context;
		initSubContext();
	}

	virtual std::unique_ptr<BaseOperator> getOperator(const QString& name) = 0;
	virtual const QString myName() = 0;
	virtual std::vector<QString> getOperators() = 0;
};