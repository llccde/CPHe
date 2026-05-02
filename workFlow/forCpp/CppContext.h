#pragma once
#include"BaseSubContext.h"
#include"qvector.h"
#include<qstring.h>
class NameMapResPack;
class LibClangContext;
class CppContext:public BaseContext{
public:
	QString mainFile;
	QVector<QString> loadedFiles;
	std::unique_ptr<LibClangContext> clangContext;
	std::unique_ptr<NameMapResPack> nameMapResPack;
	virtual ~CppContext();
};
class CppContextManager :public BaseContextManager {
public:
	std::unique_ptr<CppContext> context;
	CppContext* getCppContext();
	BaseContext* getTopOne() override;

	void newScope() override;

	void endScope() override;

};