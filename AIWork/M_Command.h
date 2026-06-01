#pragma once
#include"qstring.h"
#include"qobject.h"
#include"qmetaenum"
#include<qmap.h>
namespace awf {
	struct LineInfo {
		bool isCommentOnly = false;
		QString commentText;
		QString rawLine;
		bool hasMutiLineCommentBegin = false;   // 该行是某个多行注释的起始行（/*）
		bool hasMutiLineCommentEnd = false;   // 该行是某个多行注释的结束行（*/）Z
	};
	class  M_OperatorTypeClass :public QObject {
		Q_OBJECT
	public:
		enum M_OperatorTypeInner {
			end,normalComment,
			msg,
			notCommand,//原始代码文本,非注释,非指令标记

			fill, 
			genBegin, genEnd, debugger,
			nameFunc,
			moduleName,
			copyPrompt,
			print,

			chat,

			ref,refFiles,
			record,recordEnd,


			max //占位符
		};
		Q_ENUM(M_OperatorTypeInner);
	};
	class ArgsClass {
		Q_GADGET            // 启用元对象功能，但不继承 QObject
	public:
		enum Arg {
			single,
			notFound,
			file,
			callLLM,
			cache,
			msg,
			symbol,
			id,
			symbolName, 
			modelName,

			//refFiles
			endWith,//like"txt|cpp|json"
			subDir,//false or true ,表示是否递归处理所有子目录
			baseFolder,//搜索范围的根目录

			refThis

		};
		Q_ENUM(Arg)         // 向元对象系统注册枚举

			// 字符串 → 枚举
		static inline Arg fromString(const QString& str) {
			const QMetaEnum metaEnum = QMetaEnum::fromType<Arg>();
			bool ok = false;
			int value = metaEnum.keyToValue(str.toUtf8().constData(), &ok);
			return ok ? static_cast<Arg>(value) : notFound;  // 缺省返回 notFound
		}

		// 枚举 → 字符串
		static inline QString toString(Arg arg) {
			const QMetaEnum metaEnum = QMetaEnum::fromType<Arg>();
			const char* key = metaEnum.valueToKey(static_cast<int>(arg));
			return key ? QString::fromUtf8(key) : QString();
		}
	};
	using Args = ArgsClass::Arg;

	using M_OperatorType = M_OperatorTypeClass::M_OperatorTypeInner;
	using MP = M_OperatorTypeClass::M_OperatorTypeInner;
	QString operatorTypeToString(M_OperatorType type);
	// 辅助解析操作符字符串到枚举Z
	M_OperatorType stringToOperatorType(const QString& opStr);
	//todo 解析设置注释起止,也可能没必要
	enum CommentState {
		singleLine,
		isBegin,
		isEnd
	};
	struct M_CommandArg {
		QString key, val;
		bool hasVal;
	};
	class M_Command {
	public:
		M_OperatorType type;
		QVector<M_CommandArg> args;//@op,arg1=xx,arg2=xx
		const QString& getArg(const QString& key);//有则返回,无则返回空值;
		bool contains(const QString& key);

		const QString& getArg(ArgsClass::Arg argType);
		bool contains(ArgsClass::Arg argType);
		QString arg;//@op arg
		//mean @fill{.....@end
		bool isLongOperator = false;
		QString toString() const;
		CommentState commentState;
		bool hasMutiLineCommentBegin = false;
		bool hasMutiLineCommentEnd = false;
	};
}