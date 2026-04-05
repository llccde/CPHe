#pragma once
#include"helpfulTypes.h"
#include"qstring.h"
QString readContentFromPosition(const CodePosition & pos);
class CodeFileReader {
	Vector<QString> content;
public:
	virtual QString readLine(int offset) = 0;
	virtual int getRowCount() = 0;


};