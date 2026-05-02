#pragma once
#include"baseOperator.h"
#include"CppOperator.h"
#include"qobject.h"
class SetCppMainFile :public CppOperator {
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};
class RunCppAnalyze :public CppOperator{
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
	~RunCppAnalyze();
};