#pragma once
#include<qvector.h>
#include<qstring.h>
#include<qdebug.h>
namespace awf {
	class ExceptionCollector {
		QVector<QString> errs;
		QVector<QString> wrns;
		QVector<QString> msgs;
	public:
		void riseErr(const QString& err) {
			errs.append(err);
		}

		bool hasErr() const {
			return !errs.empty();
		}

		void riseWrn(const QString& wrn) {
			wrns.append(wrn);
		}

		bool hasWrn() const {
			return !wrns.empty();
		}

		void riseMsg(const QString& msg) {
			msgs.append(msg);
		}

		bool hasMsg() const {
			return !msgs.empty();
		}

		// 打印所有收集的信息，使用彩色字
		void printAll() const {
			// ANSI 颜色代码
			const QString red = "\033[31m";
			const QString yellow = "\033[33m";
			const QString green = "\033[32m";
			const QString reset = "\033[0m";

			// 打印错误（红色）
			for (const QString& err : errs) {
				qDebug().noquote() << red + "[ERROR]" + reset << err;
			}

			// 打印警告（黄色）
			for (const QString& wrn : wrns) {
				qDebug().noquote() << yellow + "[WARNING]" + reset << wrn;
			}

			// 打印普通消息（绿色）
			for (const QString& msg : msgs) {
				qDebug().noquote() << green + "[INFO]" + reset << msg;
			}

			// 如果没有任何信息，输出提示
			if (errs.isEmpty() && wrns.isEmpty() && msgs.isEmpty()) {
				qDebug().noquote() << "No errors, warnings, or messages to display.";
			}
		}
		QString toString() const {
			QStringList lines;

			for (const QString& err : errs) {
				lines << "[ERROR] " + err;
			}
			for (const QString& wrn : wrns) {
				lines << "[WARNING] " + wrn;
			}
			for (const QString& msg : msgs) {
				lines << "[INFO] " + msg;
			}

			if (lines.isEmpty()) {
				return "No errors, warnings, or messages to display.";
			}

			return lines.join("\n");
		}
	};
}
