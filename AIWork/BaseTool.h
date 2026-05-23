#pragma once
#include <QVector>
#include <QString>
#include <QDebug>   // 提供 qWarning
namespace awf {
    inline QString readFileContents(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 打开失败，返回空字符串（或根据需要抛出异常）
            return QString();
        }

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        QString content = in.readAll();
        file.close();
        return content;
    }
    inline QString getFirstLine(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};

        // 直接从设备读取一行
        QTextStream stream(&file);
        QString line = stream.readLine();  // readLine 自动去掉换行符

        file.close();
        return line;
    }
    inline QVector<QString> extract(const QString& begin, const QString& end, const QVector<QString>& data) {
        bool lookingBegin = true;
        QVector<QString> result;
        QString current;

        for (const auto& sub : data) {
            if (lookingBegin) {
                if (sub.trimmed() == begin) {
                    lookingBegin = false;   // 找到开始标记，进入收集模式
                }
            }
            else {
                if (sub.trimmed() == end) {
                    lookingBegin = true;    // 找到结束标记，退出收集模式
                    if (current.endsWith('\n'))
                        current.chop(1);    // 移除最后多余的换行符
                    // 可选：跳过空块 if (!current.isEmpty())
                    result.append(current);
                    current.clear();
                    continue;
                }
                current.append(sub);
                current.append('\n');
            }
        }

        if (!lookingBegin) {
            qWarning() << "not closed data when extract";
        }
        return result;
    }
    inline QVector<int> extractDecimalNumbers(const QString& text)
    {
        QVector<int> numbers;
        int currentNumber = 0;
        bool inNumber = false;

        for (const QChar& ch : text) {
            if (ch.isDigit()) {
                currentNumber = currentNumber * 10 + ch.digitValue();
                inNumber = true;
            }
            else {
                if (inNumber) {
                    numbers.append(currentNumber);
                    currentNumber = 0;
                    inNumber = false;
                }
            }
        }

        // 如果文本以数字结尾，确保最后一个数字被添加
        if (inNumber) {
            numbers.append(currentNumber);
        }

        return numbers;
    }
}
