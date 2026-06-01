#pragma once
#include"assert.h"
#include <QWidget>
class WorkFlowContext;
class PSCWidget {
protected:
	WorkFlowContext* context;
public:
	void setContext(WorkFlowContext* wfc) {
		assert(wfc != nullptr);
		context = wfc;
	}
	virtual ~PSCWidget() {
	}
};