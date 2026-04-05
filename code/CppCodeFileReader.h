#pragma once
#include"CodeFileReader.h"
#include"qvector.h"
#include"qstring.h"
#include"helpfulTypes.h"
class CppCodeFileReader :public CodeFileReader {
	// 通过 CodeFileReader 继承
	
	Vector<QString> buffer;
	bool formatIndent = false;
	int indentLength = 4;
	CodePosition pos;
public:
	
	QString readLine(int offset) override;
	int getRowCount() override;
	CppCodeFileReader(bool formatIndent,CodePosition pos, int indentLength);
private:
	void readLineRawIntoBuffer();
	void handleIndent();

};