#pragma once
#include"assert.h"
class CppContextManager;
class CppOperator {
public:
	CppContextManager* cppContextManager = nullptr;
	void setCppContextManager(CppContextManager* cm) {
		assert(cm != nullptr);
		this->cppContextManager = cm;
	}
	CppContext* getCppContext() {
		assert(cppContextManager != nullptr);
		assert(cppContextManager->context);
		return cppContextManager->context.get();
	}
	virtual ~CppOperator() {
	}
};