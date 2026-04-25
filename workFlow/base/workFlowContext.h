#pragma once
#include<qmap.h>
#include"Intent.h"
using VarName = QString;
class WorkFlowController;
class WorkFlowContext {
public:
	QMap<QString, QString> StrVars;
	QMap<QString, QVector<QString>> codePageVars;
	QMap<QString, int> intVars;
	QMap<QString, float> floatVars;
	WorkFlowController* workFlowController = nullptr;
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
};