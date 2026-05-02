#pragma once
#include"TaskFactory.h"
#include"qvector.h"
#include"qobject.h"
#include <QMetaEnum>
class CppContextManager;
class CppTaskFactory :public QObject,public TaskFactory {
	Q_OBJECT
public:
	enum CppTasks {
		setCppMainFile,
		getCppSymbol,
		loadCppFile,
		runCppAnalyze
	};
	CppContextManager* cppContext = nullptr;
	Q_ENUM(CppTasks);
	CppTaskFactory();
	// 通过 TaskFactory 继承
	std::unique_ptr<BaseOperator> getOperator(const QString& name) override;
	const QString myName() override;
	std::vector<QString> getOperators() override;

	// 通过 TaskFactory 继承
	void initSubContext() override;
};