#pragma once
#include"baseOperator.h"
class newScopeOperator :public BaseOperator {
	// 通过 BaseOperator 继承
	void handleIntent(QVector<Intent*>, WorkFlowContext*) override;
	void BeRememberAsIntent(Intent*, WorkFlowContext*) override;
};