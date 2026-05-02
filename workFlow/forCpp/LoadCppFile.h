#pragma once
#include"baseOperator.h"
#include"CppOperator.h"
class LoadCppFile :public CppOperator{
public:
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};