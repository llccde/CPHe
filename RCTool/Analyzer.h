#pragma once
#include "RecordCollector.h"
#include <QString>
#include <QObject>
#include <QMenu>
#include <QMetaEnum>
#include "ExceptionCollector.h"
#include <iostream>
#include <QClipboard>

class Analyzer : public QObject {
    Q_OBJECT
public:
    awf::ExceptionCollector ec;
    enum RefType {
        refInc,
        nullType
    };
    Q_ENUM(RefType)
        QString workingFolder;
    QDir workingDir;
    QVector<QString> args;

    Analyzer(int argc, char* argv[])
    {
        workingFolder = QDir().currentPath();
        workingDir = QDir(workingFolder);
        for (size_t i = 0; i < argc; i++)
        {
            args.append(QString(argv[i]));
        }
    }

    void argNumNotMatch(int should, int act) {
        ec.Err(QString("参数数量不匹配,期望为:%1,实际为:%2").arg(should).arg(act));
    }

    QString abs(QString filePath) {
        return workingDir.absoluteFilePath(filePath);
    }

    QString rel(QString filePath) {
        return workingDir.relativeFilePath(abs(filePath));
    }

private:
    // 抽取的公共文件处理逻辑：
    // 打开文件，逐行输出至std::cout，并按指定格式将文件头与内容追加到outResult
    // 失败时已通过ec.Err记录错误，返回false
    bool processFileToOutput(const QString& fileAbsPath, const QString& fileRelPath,
        const QString& headerFormat, QString& outResult) {
        QFile file(fileAbsPath);
        if (!file.open(QFile::ReadOnly)) {
            ec.Err(QString("无法打开文件:%1").arg(fileRelPath));
            return false;
        }
        QString content;
        while (!file.atEnd()) {
            QString line = file.readLine(); // 保留行尾换行符
            content += line;
            std::cout << line.toStdString();
        }
        // 文件头 + 文件内容，并在末尾添加额外换行作为块分隔
        outResult += headerFormat.arg(fileRelPath) + "\n" + content + "\n";
        return true;
    }

public:
    void work() {
        QVector<QString> opArgs;
        RefType type = nullType;
        bool recivedType = false;
        bool reciveOpArgs = false;

        for (int idx = 1; idx < args.size(); ++idx) {
            const QString& i = args[idx];
            if (idx == 1) {
                bool succ = false;
                type = static_cast<RefType>(
                    QMetaEnum::fromType<RefType>().keyToValue(i.toUtf8().constData(), &succ));
                recivedType = true;
                reciveOpArgs = true;
                if (!succ) {
                    ec.Err(QString("未知操作类型:%1").arg(i));
                    return;
                }
            }
            else {
                if (reciveOpArgs) {
                    opArgs.append(i);
                }
                else {
                    ec.Err("过多的参数");
                    return;
                }
            }
        }

        switch (type) {
        case Analyzer::refInc: {
            if (opArgs.count() != 1) {
                argNumNotMatch(1, opArgs.size());
                return;
            }
            BaseRefFeature brf;
            QFile f(abs(opArgs[0]));
            if (!f.exists()) {
                ec.Err(QString("文件不存在:%1").arg(QFileInfo(opArgs[0]).absoluteFilePath()));
                return;
            }
            brf.workingFolder = workingFolder;
            auto incs = brf.refIncludeFiles(f);
            QString outResult;

            // 处理所有被引用的包含文件（失败时仅记录错误，继续处理后续）
            for (const auto& incAbsPath : incs) {
                processFileToOutput(incAbsPath, rel(incAbsPath),
                    QStringLiteral("file:[%1]"), outResult);
            }

            // 处理主文件（失败时记录错误并直接返回）
            if (!processFileToOutput(abs(opArgs[0]), rel(opArgs[0]),
                QStringLiteral("mainFile:[%1]"), outResult)) {
                return;
            }

            QApplication::clipboard()->setText(outResult);
            break;
        }
        case Analyzer::nullType:
            break;
        default:
            break;
        }
    }
};