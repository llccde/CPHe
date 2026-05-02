#pragma once
#include<vector>
#include<TaskFactory.h>
#include"baseOperator.h"
#include"qmap.h"
#include"Intent.h"
class Intent;
template<class T>
struct TaskNameInFactory {
	QString factoryName;
	std::vector<T> items;
	const T& operator[](const int index) const{
		assert(index >= 0 && index < items.size());
		return items[index];
	}
	TaskNameInFactory(QString name):factoryName(name) {}
	~TaskNameInFactory();
};
class TaskLauncher {
	WorkFlowContext* context;
	std::unique_ptr<BaseOperator> curentOperator;
	std::vector<std::unique_ptr<Intent>> currentIntents;
public:
	TaskLauncher(WorkFlowContext* context,std::unique_ptr<BaseOperator> curentOperator)
		: context(context), curentOperator(std::move(curentOperator))
	{
	}
	void AddIntent(QString describe);
	void launch();
	void store(QString des);
	~TaskLauncher();
};
class TaskManager {
	std::vector<std::unique_ptr<TaskFactory>> factorys;
	std::unique_ptr<WorkFlowContext> context;
	QMap<QString, TaskFactory*> operatorNames;
public:
	TaskManager();
	~TaskManager();
	void registTaskFactory(std::unique_ptr<TaskFactory>);
	std::unique_ptr<TaskLauncher> prepareOperator(QString name);
	std::vector<TaskNameInFactory<QString>> getAllOperator();
};

