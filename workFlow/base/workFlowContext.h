#pragma once
#include<qmap.h>
#include"Intent.h"
#include<vector>
#include<memory>
#include"BaseSubContext.h"
using VarName = QString;
class WorkFlowController;
class BaseContextManager;
class WorkFlowContext {
public:
	QMap<QString, QString> StrVars;
	QMap<QString, QVector<QString>> codePageVars;
	QMap<QString, int> intVars;
	QMap<QString, float> floatVars;
	std::vector<std::unique_ptr<BaseContextManager>> contextManagers;
	WorkFlowController* workFlowController = nullptr;
	void addContextManagers(std::unique_ptr<BaseContextManager> c) {
		contextManagers.push_back(std::move(c));
	};
	void newRunScope();
	QString* getStrByKey(QString key) {
		if (StrVars.contains(key)) {
			return &StrVars[key];
		}
		else{
			return nullptr;
		}
	}
	int* getIntByKey(QString key) {
		if (intVars.contains(key)) {
			return &intVars[key];
		}
		else {
			return nullptr;
		}
	}
	float* getFloatByKey(QString key) {
		if (floatVars.contains(key)) {
			return &floatVars[key];
		}
		else {
			return nullptr;
		}
	}
	QVector<QString>* getCodePageByKey(QString key) {
		if (codePageVars.contains(key)) {
			return &codePageVars[key];
		}
		else {
			return nullptr;
		}
	}
	~WorkFlowContext();
};