#pragma once
#include"assert.h"
#include"baseOperator.h"
class CppContextManager;
class CppContext;
class CppOperator:public BaseOperator{
public:
	CppContextManager* cppContextManager = nullptr;
	void setCppContextManager(CppContextManager* cm) {
		assert(cm != nullptr);
		this->cppContextManager = cm;
	}
	CppContext* getCppContext();
	virtual ~CppOperator() {
	}
};