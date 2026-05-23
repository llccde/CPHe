#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include "ExceptionCollector.h"
#include"qobject.h"
#include"M_Command.h"
namespace awf {

	
	class Interpreter {
		ExceptionCollector& ec;
	public:
		Interpreter(ExceptionCollector& ec) :ec(ec) {}
		void riseWarning(const QString& wrn) {
			ec.riseWrn(wrn);
		}
		void loadFile(QString filePath);
		/*is注释行(row)->bool{
			return (行不包含任何非注释代码(row))
		}
		if(is注释行(row)){
			if(注释内容以@开头(row)){
				if(结构为 "@op ?argData"){
					return{type=op}
				}if(结构为 "@op{" ){
					return{type=op,islongop=true}
				}if(结构为 "@op,arg1=s1,argx=sx"){
					return {type = op,args = {arg1=s1,argx=sx}}
				}
			}else{
				return {type = normalComment,arg=注释内容(row)}
			}
		}else{
			return {type = notCommand,arg=行内容(row)}
		}*/
		M_Command getCommandOf(int row);
		bool isCommandComment(int row);
		QString getSource(int row);
		int rowCount();

	private:
		QVector<LineInfo> mLines;
		mutable QHash<int, M_Command> mCommandCache; // 缓存解析结果

		void parseArguments(const QString& argPart, QMap<QString, QString>& args);
	};
}