#pragma once
#include"baseOperator.h"
#include"CppOperator.h"
class LoadFile :public BaseOperator,public CppOperator{
public:
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};