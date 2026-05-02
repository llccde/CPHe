#pragma once

#include <memory>
#include <functional>
#include <QString>
#include <QVector>
#include <QMap>
#include <QDebug>
#include <QFile>

#include "PSCTranslater.h"   // 假设该头文件定义了 PSCCommand, PSCCommandArg 等
class RunTimeErrorCollector {
public:
    QVector<QString> errors;

    void put(const QString& msg) {
        errors.append(msg);
    }

    bool hasError() const {
        return !errors.isEmpty();
    }

    void output() const {
        for (const auto& e : errors) {
            qDebug() << e << "\n";
        }
    }
    void reset() {
        errors.clear();
    }
};
using OPResult = QString;
static const QString defaultUDI = "default";
// 修正类名和成员名拼写：Receiver
class PSCReceiver {
    
protected:
    QMap<QString, OPResult> data;
public:
    
    QString defaultResult = "";   // 保留用于兼容，实际 get 中已改用静态空串
    PSCReceiver() {
        receive(defaultUDI, "");
    }
    // 接收数据（拼接或覆盖由子类决定）
    virtual void receive(QString userDefID, const OPResult& result) {
        data.insert(userDefID, result);
    }

    // 获取数据，不存在时返回静态空串的 const 引用
    virtual const OPResult& get(QString userDefID) const {
        static const QString empty;
        auto it = data.find(userDefID);
        if (it != data.end()) {
            return *it;
        }
        return empty;
    }

    // 判断是否包含指定 userDefID
    virtual bool has(QString userDefID) const {
        return data.contains(userDefID);
    }

    virtual ~PSCReceiver() = default;
};

// 拼接式接收器
class AppendPSCReceiver : public PSCReceiver {
public:
    void receive(QString userDefID, const OPResult& result) override {
        if (has(userDefID)) {
            data[userDefID] += result;   // 拼接
        }
        else {
            data.insert(userDefID, result);
        }
    }
};
class EmptyPSCReceiver : public PSCReceiver {
public:
    void receive(QString userDefID, const OPResult& result) override {
        return;
    }
    virtual bool has(QString userDefID) const {
        return true;
    }
    virtual const OPResult& get(QString userDefID) const {
        static const QString empty;
        return empty;
    }
};

// 上下文：管理多个接收器
class PSCContext {
public:
    std::map<QString, std::unique_ptr<PSCReceiver>> receivers;

    // 获取指定接收器的指定项的值（副本）
    QString get(QString receiverName, QString userDefID) {
        auto it = receivers.find(receiverName);
        if (it != receivers.end()) {
            return it->second->get(userDefID);
        }
        return {};
    }

    // 向指定接收器写入数据
    void write(QString receiverName, QString userDefID, const QString& value) {
        auto it = receivers.find(receiverName);
        if (it != receivers.end()) {
            it->second->receive(userDefID, value);
        }
    }

    // 注册接收器
    void registe(std::unique_ptr<PSCReceiver> recv, QString name) {
        if (recv && receivers.find(name) == receivers.end()) {
            receivers[name] = std::move(recv);
        }
    }

    // 检查接收器是否存在
    bool contains(QString name) const {
        return receivers.find(name) != receivers.end();
    }

    // 获取接收器裸指针（谨慎使用）
    PSCReceiver* getReceiver(QString name) {
        auto it = receivers.find(name);
        return (it != receivers.end()) ? it->second.get() : nullptr;
    }
};

class PSCOperator {
    RunTimeErrorCollector* collector = nullptr;   // 建议改名
    PSCContext* context = nullptr;
protected:
    virtual QString call_impl(QVector<QString>& args) = 0;

public:
    QString call(QVector<QString>& args, RunTimeErrorCollector& err, PSCContext& ctx) {
        collector = &err;
        context = &ctx;
        return call_impl(args);   // 关键修复
    }
    void error(const QString& err) {
        collector->put(err);
    }
    QString redCtx(const QString& recv, const QString& udi) {
        if (context->contains(recv)) {
            if (context->getReceiver(recv)->has(udi)) {
                return context->get(recv, udi);
            }
        }
        return {};   // 关键修复
    }
    virtual ~PSCOperator() = default;
};
// Lambda 操作符
class PSCLambdaOperator : public PSCOperator {
    using Callable = std::function<QString(const QVector<QString>& args,PSCOperator* _this)>;
    Callable callback;
public:
    PSCLambdaOperator(Callable ca) : callback(std::move(ca)) {}

    QString call_impl(QVector<QString>& args) override {
        if (callback)
            return callback(args,this);
        return {};
    }

    ~PSCLambdaOperator() override = default;

    static std::unique_ptr<PSCLambdaOperator> getByLambda(Callable lambda) {
        return std::make_unique<PSCLambdaOperator>(std::move(lambda));
    }
};

// 运行时错误收集器


static struct Oprerators{
    QString join = "join";

} opNames;
static struct Receivers {
    QString empty = "empty";
    QString outPut = "output";
    QString env = "env";
} recvNames;
// 解释器
class PSCInterpreter {
public:
    std::unique_ptr<PSCTranslater> translater;   // 假设 PSCTranslater 类已定义
    PSCContext context;
    std::map<QString, std::unique_ptr<PSCOperator>> operators;
    PSCInterpreter() {
        // 注册默认输出接收器（拼接模式）
        context.registe(std::make_unique<AppendPSCReceiver>(), recvNames.outPut);
        context.registe(std::make_unique<EmptyPSCReceiver>(), recvNames.empty);
        context.registe(std::make_unique<PSCReceiver>(), recvNames.env);
        operators.insert({ 
            opNames.join,
            PSCLambdaOperator::getByLambda([](const QVector<QString>& args,PSCOperator*) {
                QString ret;
                for(const auto&i:args){
                    ret.append(i + "\n");
                }
                return ret;
            }) 
        });
        translater = std::make_unique<PSCTranslater>();
    }

    // 执行文件
    void runFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "file not found or cannot open:" << path << "\n";
            return;
        }

        QVector<QString> codes;
        while (!file.atEnd()) {
            codes.append(QString::fromUtf8(file.readLine())); // 显式处理编码
        }
        interpret(codes);
    }

    // 解释执行代码行列表
    void interpret(const QVector<QString>& code) {
        RunTimeErrorCollector err;
        for (const auto& line : code) {
            if (line.trimmed().isEmpty()) continue; // 跳过空行
            run(line, err);
            if (err.hasError()) {
                qDebug() << "Error occurred at line:" << line;
                err.output();
                break;   // 遇到错误停止执行后续行（策略可调整）
            }
        }
    }

    // 执行单行代码，错误信息收集到 err 中
    void run(const QString& codeLine, RunTimeErrorCollector& err) {
        PSCCommand common;   // 假设 PSCCommand 有默认构造函数且 valid == false

        // 解析指令
        if (codeLine.startsWith('@')) {
            common = translater->translate(codeLine);
        }
        else {
            // 纯文本直接作为输出
            common = PSCCommand(
                true, opNames.join, recvNames.outPut, defaultUDI,
                { PSCCommandArg(true, PSCCommandArg::rawStr, codeLine, "") }
            );
        }

        // 校验指令有效性
        if (!common.valid) {
            err.put("Invalid command: " + codeLine);
            translater->outputErr();   // 原有错误输出保留
            return;
        }

        // 检查操作符是否存在
        if (!operators.count(common.op)) {
            err.put("Unknown operator: " + common.op);
            return;
        }

        // 默认接收器处理
        if (common.recv.isEmpty()) {
            common.recv = recvNames.empty;
        }
        if (!context.contains(common.recv)) {
            err.put("Receiver not found: " + common.recv);
            return;
        }

        // 默认接收器内 ID 处理
        if (common.recvUDI.isEmpty()) {
            common.recvUDI = defaultUDI;
        }

        // 处理 args，解析出实际字符串列表
        QVector<QString> resolvedArgs;
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
                // 从上下文中取出调用来源接收器的值
                resolvedArgs.append(context.get(arg.body, arg.argForCall));
            }
            else if (arg.type == PSCCommandArg::rawStr) {
                resolvedArgs.append(arg.body);
            }
            else {
                // 其他类型暂不处理，记录错误
                err.put("Unsupported argument type in command: " + codeLine);
                return;
            }
        }
        
        // 调用操作符并获取结果
        QString result = operators[common.op]->call(resolvedArgs,err);

        // 将结果写入目标接收器
        context.write(common.recv, common.recvUDI, result);
    }
};