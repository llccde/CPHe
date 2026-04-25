#pragma once
#include<qstring.h>
class WorkFlowContext;
class Intent {
public:
	enum MetaType {
		rawStr,
		varName
	};
	enum VarType {
		intVar,strVar,floatVar
	};
	MetaType isStr;
	VarType varType;

	QString describe;
	WorkFlowContext* mainContext;
	bool operator<(const Intent& other) const {
		return this->describe < other.describe;
	}
	bool operator>(const Intent& other) const {
		return this->describe > other.describe;
	}
	bool operator==(const Intent& other) const {
		return this->describe == other.describe;
	}
	static Intent getIntentByDescribe(QString des, WorkFlowContext* ctx) {
		return Intent(des,ctx);
	}
	virtual QString getData() {
		assert(false);
		return "";
	}

	//暂时不打算使用这个方法
	virtual QString resolveIntent();
	virtual ~Intent() {
	}
protected:
	Intent(const QString& describe, WorkFlowContext* ctx)
		: describe(describe),mainContext(ctx){
	}
	
};