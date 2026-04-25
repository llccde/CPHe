#include "CppContext.h"
#include"NameTree.h"
CppContext* CppContextManager::getCppContext()
{
	return context.get();
};
BaseContext* CppContextManager::getTopOne()
{
	return context.get();
}

void CppContextManager::newScope()
{
	if (context == nullptr) {
		context.reset(new CppContext());
	}
}

void CppContextManager::endScope()
{
	return;
}
