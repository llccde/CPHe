#pragma once
#include"baseOperator.h"
#include"CppOperator.h"
class SetCppMainFile :public BaseOperator, public CppOperator {
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};
class RunCppAnalyze :public BaseOperator, public CppOperator{
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};