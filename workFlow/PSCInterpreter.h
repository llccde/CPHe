#pragma once

#include <memory>
#include <functional>
#include <QString>
#include <QVector>
#include <QMap>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <iostream>
#include "CppFileAnalyzer.h"
#include "PSCTranslater.h" 
#include"PSCBuildingOperators.h"
#include"PSCVar.h"
#include"RuntimeErrcollector.h"
#include"PSCReceivers.h"
#include"PSCContext.h"
#include"PSCOperators.h"
#include"PSCVarDrefTool.h"
static struct Operators {
    QString join = "join";
    QString printOutPut = "printOut";
    QString setFile = "setFile";
    QString getDef = "getDef";
    QString getDecl = "getDecl";
    QString getDeclWithDepth = "getDeclWithDepth";
} opNames;

static struct Receivers {
    QString empty = "empty";
    QString outPut = "output";
    QString env = "env";
    QString vars = "vars";
} recvNames;

// ---------------------------- PSCInterpreter ----------------------------
class PSCInterpreter {
public:
    std::unique_ptr<PSCTranslater> translater;
    PSCContext context;
    std::map<QString, std::unique_ptr<PSCOperator>> operators;

    PSCInterpreter() {
        // 注册默认接收器（不变）
        context.registe(std::make_unique<AppendPSCReceiver>(), recvNames.outPut);
        context.registe(std::make_unique<EmptyPSCReceiver>(), recvNames.empty);
        context.registe(std::make_unique<PSCReceiver>(), recvNames.env);
        context.registe(std::make_unique<PSCReceiver>(), recvNames.vars);

        // join 操作符
        operators.insert({
            opNames.join,
            PSCLambdaOperator::getByLambda(&builtin_join)
            });

        // printOut 操作符
        auto printOutOp = PSCLambdaOperator::getByLambda(&builtin_printOut);
        printOutOp->argNoMore(0);
        printOutOp->setName(opNames.printOutPut);
        printOutOp->setReturnType(PSCVarType::nullVar);
        operators.insert({ opNames.printOutPut, std::move(printOutOp) });

        // setFile 操作符
        auto setFileOp = PSCLambdaOperator::getByLambda(&builtin_setFile);
        setFileOp->argExact(1);
        setFileOp->setName(opNames.setFile);
        setFileOp->setArgTypes({PSCVarType::rawStr});
        setFileOp->setReturnType(PSCVarType::nullVar);
        operators.insert({ opNames.setFile, std::move(setFileOp) });

        // getDef 操作符
        auto getDefOp = PSCLambdaOperator::getByLambda(&builtin_getDef);
        getDefOp->argAny();
        getDefOp->setName(opNames.getDef);
        getDefOp->setArgTypes({ PSCVarType::rawStr });
        getDefOp->setReturnType(PSCVarType::CXCursor);
        operators.insert({ opNames.getDef, std::move(getDefOp) });

        auto getDeclWithDepthOp = PSCLambdaOperator::getByLambda(&builtin_getDefDepth);
        
        translater = std::make_unique<PSCTranslater>();
    }


    void runFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "file not found or cannot open:" << path << "\n";
            return;
        }
        QVector<QString> codes;
        while (!file.atEnd())
            codes.append(QString::fromUtf8(file.readLine()));
        interpret(codes);
    }

    void interpret(const QVector<QString>& code) {
        RunTimeErrorCollector err;
        context.collector = &err;
        for (const auto& line : code) {
            if (line.trimmed().isEmpty()) continue;
            run(line, err);
            if (err.hasError()) {
                qDebug() << "Error occurred at line:" << line;
                err.output();
                break;
            }
        }
    }

    void run(const QString& codeLine, RunTimeErrorCollector& err) {
        PSCCommand common;
        if (codeLine.startsWith('@')) {
            common = translater->translate(codeLine);
        }
        else {
            // 纯文本视为输出
            QVector<PSCCommandArg> args;
            args.append(PSCCommandArg(true, PSCCommandArg::rawStr, codeLine, ""));
            if (!codeLine.endsWith("\n"))
                args.append(PSCCommandArg(true, PSCCommandArg::rawStr, "\n", ""));
            common = PSCCommand(true, opNames.join, recvNames.outPut, defaultUDI, args);
        }

        if (!common.valid) {
            err.put("Invalid command: " + codeLine);
            translater->outputErr();
            return;
        }
        if (!operators.count(common.op)) {
            err.put("Unknown operator: " + common.op);
            return;
        }
        if (common.recv.isEmpty()) common.recv = recvNames.empty;
        if (!context.contains(common.recv)) {
            err.put("Receiver not found: " + common.recv);
            return;
        }
        if (common.recvUDI.isEmpty()) common.recvUDI = defaultUDI;

        // 将指令参数解析为 PSCVar 列表
        QVector<PSCVar> resolvedArgs;
        for (const auto& arg : common.args) {
            if (!arg.valid) {
                err.put("Invalid argument in command: " + codeLine);
                return;
            }
            if (arg.type == PSCCommandArg::call) {
                if (arg.argForCall.isEmpty()) {
                    err.put("Missing argument name for call type in command: " + codeLine);
                    return;
                }
                // 从上下文读取 PSCVar
                resolvedArgs.append(context.get(arg.body, arg.argForCall));
            }
            else if (arg.type == PSCCommandArg::rawStr) {
                resolvedArgs.append(PSCVar(arg.body, PSCVarType::rawStr));
            }
            else {
                err.put("Unsupported argument type in command: " + codeLine);
                return;
            }
        }

        // 调用操作符，得到 PSCVar 结果
        PSCVar result = operators[common.op]->call(resolvedArgs, err, context);
        // 将结果写入目标接收器
        context.write(common.recv, common.recvUDI, result);
    }
};