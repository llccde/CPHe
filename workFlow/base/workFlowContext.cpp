#include"workFlowContext.h"
#include"BaseSubContext.h"
void WorkFlowContext::newRunScope()
{
	for (auto &i:contextManagers)
	{
		i->newScope();
	}
}
WorkFlowContext::~WorkFlowContext()
{
}
