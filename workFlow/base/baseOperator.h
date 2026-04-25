#pragma once
#include<assert.h>
#include<qvector.h>
class Intent;
class WorkFlowContext;
class BaseOperator {
public:
	virtual void handleIntent(QVector<Intent*>, WorkFlowContext*) = 0;
	
	virtual void BeRememberAsIntent(Intent*, WorkFlowContext*) = 0;
};