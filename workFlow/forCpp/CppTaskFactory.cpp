#include "CppTaskFactory.h"
#include"CppOperator.h"
#include"ManyCppOperators.h"
#include"LoadCppFile.h"
#include"GetCppSymbol.h"
#include"CppContext.h"
#include"workFlowContext.h"
CppTaskFactory::CppTaskFactory()
{
	
}
std::unique_ptr<BaseOperator> CppTaskFactory::getOperator(const QString& name)
{
	QMetaEnum metaEnum = QMetaEnum::fromType<CppTasks>();
	CppTasks taskId = static_cast<CppTasks>(metaEnum.keyToValue(name.toUtf8().data()));
	CppOperator* op = nullptr;
	switch (taskId)
	{
	case CppTaskFactory::setCppMainFile:
		op = new SetCppMainFile();
		break;
	case CppTaskFactory::getCppSymbol:
		op = new GetCppSymbolOperator();
		break;
	case CppTaskFactory::loadCppFile:
		op = new LoadCppFile();
		break;
	case CppTaskFactory::runCppAnalyze:
		op = new RunCppAnalyze();
		break;
	default:
		break;
	}
	if (op) {
		op->setCppContextManager(cppContext);
	}
	return std::unique_ptr<BaseOperator>(op);
}

const QString CppTaskFactory::myName()
{
	return "CPPtaskFactory";
}

std::vector<QString> CppTaskFactory::getOperators()
{
	std::vector<QString> names;
	QMetaEnum metaEnum = QMetaEnum::fromType<CppTasks>();
	for (size_t i = 0; i < metaEnum.keyCount(); i++)
	{
		names.push_back(metaEnum.valueToKey(i));
	}
	return names;
}

void CppTaskFactory::initSubContext()
{
	cppContext = new CppContextManager();
	mainContext->addContextManagers(std::unique_ptr<CppContextManager>(cppContext));

}
