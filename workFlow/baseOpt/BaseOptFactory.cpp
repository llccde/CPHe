#include "BaseOptFactory.h"
#include"qmetaobject.h"
#include"ManyBaseOpts.h"
void BaseOptFactory::initSubContext()
{
}


std::unique_ptr<BaseOperator> BaseOptFactory::getOperator(const QString& name)
{
	QMetaEnum metaEnum = QMetaEnum::fromType<BaseTasks>();
	BaseTasks opt = (BaseTasks)metaEnum.keyToValue(name.toUtf8().data());
	switch (opt)
	{
	case BaseOptFactory::newScope:
		return std::unique_ptr<BaseOperator>(new newScopeOperator());
		break;
	default:
		break;
	}
	return nullptr;
}

const QString BaseOptFactory::myName()
{
	return "baseOpts";
}

std::vector<QString> BaseOptFactory::getOperators()
{
	std::vector<QString> names;
	QMetaEnum metaEnum = QMetaEnum::fromType<BaseTasks>();
	for (size_t i = 0; i < metaEnum.keyCount(); i++)
	{
		names.push_back(metaEnum.valueToKey(i));
	}
	return names;
}
