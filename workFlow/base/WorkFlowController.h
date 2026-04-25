#pragma once
#include"qstring.h"
#include"qdebug.h"
class WorkFlowController {
public:
	void reportError(QString msg) {
		qFatal()<<msg;
		#ifdef _WIN32
		__debugbreak();
		#else
		__builtin_trap();
		#endif
	}
};