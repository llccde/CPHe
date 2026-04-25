#include "LoadFile.h"
#include"workFlowContext.h"
#include"CppContext.h"
#include"Intent.h"

void LoadFile::handleIntent(QVector<Intent*>intents, WorkFlowContext* ctx)
{
	for (auto i : intents)
	{
		if (auto str = ctx->getStrByKey(i->getData())) {
			cppContextManager->getCppContext()->loadedFiles.append(*str);
		}
		else{
			cppContextManager->getCppContext()->loadedFiles.append(i->getData());
		}
	}
}

void LoadFile::BeRememberAsIntent(Intent*, WorkFlowContext*)
{
}
