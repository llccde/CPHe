#pragma once

#include <QString>
#include <QVector>
#include "ExceptionCollector.h"
#include "M_Command.h"
namespace awf {

	class ClangTool {
		ExceptionCollector& ec;
	public:
		ClangTool(ExceptionCollector& ec) :ec(ec) {}
		QVector<LineInfo> analyzeFile(const QString& filePath);
		QString getAllTheFile(const QString& p);
		void FormatTab(QVector<QString>& code, const QString refObj);
		QString clearSingleLineBreak(const QString& code);
		void clearLineBreak(QVector<QString>& code);
		QString SameTab(const QString& ori, const QString& ref);
		QVector<QString>getSymbolDef(const QString& code, const QString& symbolName, const QString& fileName = " E:/input.cpp");
	};

} // namespace awf