#include"ManyBaseOpts.h"
#include"workFlowContext.h"
void newScopeOperator::handleIntent(QVector<Intent*>, WorkFlowContext* wfc)
{
	wfc->newRunScope();
}

void newScopeOperator::BeRememberAsIntent(Intent*, WorkFlowContext*)
{
}
