#include "Intent.h"
#include"workFlowContext.h"
#include"WorkFlowController.h"
QString Intent::resolveIntent()
{
    if (isStr ==rawStr) {
        return describe;
	}
	else{
		switch (varType)
		{
		case Intent::intVar:
			if (auto i = mainContext->getIntByKey(describe)) {
				return QString::number(*i);
			}
			break;
		case Intent::strVar:
			if (auto i = mainContext->getStrByKey(describe)) {
				return *i;
			}
			break;
		case Intent::floatVar:
			if (auto i = mainContext->getFloatByKey(describe)) {
				return QString::number(*i);
			}
			break;
		default:
			break;
		}
		mainContext->workFlowController->reportError(this->describe + "{resolve failed}");
		return"";
	}
}
