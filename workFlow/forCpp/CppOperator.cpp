#include"CppOperator.h"
#include"CppContext.h"
CppContext* CppOperator::getCppContext() {
	assert(cppContextManager != nullptr);
	assert(cppContextManager->context);
	return cppContextManager->context.get();
}