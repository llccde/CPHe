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
	public:
		// 判断在 b 行之后插入一行是否会处于多行注释块内
		// b 可以为 -1（文件最开头）或 rowCount()-1（最后一行之后）
		bool isCommentBlockAfter(int b) const;

		// 获取某一行的父指令行号（即包含它的最近的长指令开始行），若无则返回 -1
		int getParentRow(int row) const;

		// 获取指定长指令开始行的所有直接子指令的行号列表（包括嵌套块的整体）
		QVector<int> getChildRows(int parentRow) const;

		// === 在类定义的 private 部分添加 ===
	private:
		QVector<bool> mInBlockAfterLine;   // 索引 i 表示处理完第 i-1 行后的注释块状态，长度为 rowCount+1
		QVector<int>  mParentRow;          // 每行的父长指令开始行索引，-1 表示顶层
	private:
		QVector<LineInfo> mLines;
		mutable QHash<int, M_Command> mCommandCache; // 缓存解析结果

		void parseArguments(const QString& argPart, QVector<M_CommandArg>& args);
	};
}