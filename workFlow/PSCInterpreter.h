#pragma once

#include <memory>
#include <functional>
#include <QString>
#include <QVector>
#include <QMap>
#include <QDebug>
#include <QFile>
#include"CppFileAnalyzer.h"
#include "PSCTranslater.h"   // 假设该头文件定义了 PSCCommand, PSCCommandArg 等
#include <QVector>
#include <QString>
#include <QDebug>
class PSCContext;
class PSCVarTypeClass :QObject {
public:
    enum PSCVarType {
        rawStr,
        codeSnippetRef,

    };
    Q_ENUM(PSCVarType);
};
using PSCVarType = PSCVarTypeClass::PSCVarType;

class PSCVar {
    QString content;
    std::function<PSCVar(PSCVar* _this)> dRefCall;
    PSCContext* ctx;
    PSCVarType type;
    PSCVar dRef() {
    }
    QString toString() {
        if (type == PSCVarType::rawStr) {
            return content;
        }
        else
        {

        }
    }
};
class RunTimeErrorCollector {
public:
    struct Msg {
        enum Level {
            msg,
            warn,
            error
        } level;
        QString content;
    };
    QVector<Msg> msgs;
    // 默认添加 error 级别消息
    void put(const QString& msg) {
        msgs.append({ Msg::error, msg });
    }
    // 添加 error 级别消息（与 put 等价，提供语义明确的接口）
    void putError(const QString& msg) {
        msgs.append({ Msg::error, msg });
    }
    void putWarn(const QString& msg) { msgs.append({ Msg::warn, msg }); }
    void putMsg(const QString& msg) { msgs.append({ Msg::msg, msg }); }
    // 是否存在至少一条 error 消息
    bool hasError() const {
        for (const auto& m : msgs) {
            if (m.level == Msg::error)
                return true;
        }
        return false;
    }
    // 打印所有消息，按级别显示不同前缀
    void output() const {
        for (const auto& e : msgs) {
            switch (e.level) {
            case Msg::error:
                qDebug() << "Error:" << e.content;
                break;
            case Msg::warn:
                qDebug() << "Warning:" << e.content;
                break;
            case Msg::msg:
                qDebug() << "Info:" << e.content;
                break;
            default:
                qDebug() << "Unknown:" << e.content;
                break;
            }
        }
    }
    // 清空所有消息
    void reset() {
        msgs.clear();
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
    struct CPPContext {
        QString MainFilePath;
        QString workingFolder;
        CppCodeAnalyzer analyzer;
        CppCodeAnalyzerResult result;
    } cppContext;
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
protected:
    enum ArgNumType {
        any,           // 任意数量
        atLeast,       // 至少 n 个
        noMoreThan,    // 最多 n 个
        onlyExactNum   // 恰好 n 个
    } argType = any;   // 默认允许任意数量

    int theExactNum = -1;

    virtual QString call_impl(QVector<QString>& args) = 0;
    virtual bool checkArg_child(QVector<QString>& args) { return true; } // 默认无额外检查

public:
    RunTimeErrorCollector* collector = nullptr;
    PSCContext* context = nullptr;
    // 配置参数数量约束
    void argAny() {
        argType = any;
    }
    void argLeast(int leastNum) {
        argType = atLeast;
        theExactNum = leastNum;
    }
    void argNoMore(int maxNum) {
        argType = noMoreThan;
        theExactNum = maxNum;
    }
    void argExact(int thNum) {
        argType = onlyExactNum;
        theExactNum = thNum;
    }
    bool checkArg(QVector<QString>& args) {
        if (!checkArg_child(args)) {
            return false;
        }
        switch (argType) {
        case any:
            break;
        case atLeast:
            if (args.size() < theExactNum) {
                error(QString("at least %1 arguments required, but %2 provided")
                    .arg(theExactNum)
                    .arg(args.size()));
                return false;
            }
            break;
        case noMoreThan:
            if (args.size() > theExactNum) {
                error(QString("no more than %1 arguments allowed, but %2 provided")
                    .arg(theExactNum)
                    .arg(args.size()));
                return false;
            }
            break;
        case onlyExactNum:
            if (args.size() != theExactNum) {
                error(QString("exactly %1 arguments required, but %2 provided")
                    .arg(theExactNum)
                    .arg(args.size()));
                return false;
            }
            break;
        }
        return true;
    }
    QString call(QVector<QString>& args, RunTimeErrorCollector& err, PSCContext& ctx) {
        collector = &err;
        context = &ctx;
        if (!checkArg(args)) {
            return {};   // 检查失败，错误已记录
        }
        return call_impl(args);
    }
    void error(const QString& msg) {
        if (collector)
            collector->putError("in operator: " + msg);
    }
    void warn(const QString& wrn) {
        if (collector)
            collector->putWarn("in operator: " + wrn);
    }
    void message(const QString& msg) {
        if (collector)
            collector->putMsg("in operator: " + msg);
    }
    QString redCtx(const QString& recv, const QString& udi) {
        if (context && context->contains(recv)) {
            auto* receiver = context->getReceiver(recv);
            if (receiver && receiver->has(udi)) {
                return context->get(recv, udi);
            }
        }
        return {};
    }

    virtual ~PSCOperator() = default;
};// Lambda 操作符
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
    QString printOutPut = "printOut";
    QString setFile = "setFile";
    QString getDef = "getDef";
    QString getDecl = "getDecl";
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
                    ret.append(i);
                }
                return ret;
            }) 
        });
        auto printOutOp = PSCLambdaOperator::getByLambda([&](const QVector<QString>& args, PSCOperator* _this) {
            std::cout << _this->redCtx(recvNames.outPut, defaultUDI).toStdString() << std::endl;
            return "";
        });
        printOutOp->argNoMore(0);
        operators.insert({
            opNames.printOutPut,
            std::move(printOutOp)
        });
        auto setFileOp = PSCLambdaOperator::getByLambda([&](const QVector<QString>& args, PSCOperator* _this) {
            const QString& filePath = args[0];
            QFileInfo fileInfo(filePath);
            if (!fileInfo.exists()) {
                _this->error(QString("setFile: file does not exist: %1").arg(filePath));
                return QString();
            }
            // 检查是否为普通文件（非目录、符号链接等）
            if (!fileInfo.isFile()) {
                _this->error(QString("setFile: path is not a regular file: %1").arg(filePath));
                return QString();
            }
            //检查文件是否可读
            if (!fileInfo.isReadable()) {
                _this->error(QString("setFile: file is not readable: %1").arg(filePath));
                return QString();
            }
            //发出警告：文件为空（可能不是致命错误，仅提示）
            if (fileInfo.size() == 0) {
                _this->warn(QString("setFile: file is empty: %1").arg(filePath));
            }
            //通过所有检查，设置路径并运行分析器
            auto& cpp = _this->context->cppContext;
            cpp.MainFilePath = filePath;
            cpp.result = cpp.analyzer.runAnalyzer(filePath);

            // 可选的日志输出
            _this->message(QString("setFile: analysis completed for %1").arg(filePath));

            return QString();  // 成功时返回空字符串
            });
        
        setFileOp->argExact(1);
        operators.insert({
            opNames.setFile,
            std::move(setFileOp)
        });
        //args like parentName1,parentName2,parentName3,finalName
        auto getDefOp = PSCLambdaOperator::getByLambda([&](const QVector<QString>& args, PSCOperator* _this) {
            auto& data = _this->context->cppContext.result;
            using Identifier = CppCodeAnalyzerResult::Identifier;

            Identifier current = data.getRoot(); // ID = 0，根节点
            for (const QString& name : args) {
                bool found = false;
                // 在当前节点的所有子节点中查找匹配的名字
                for (Identifier childId : data.children[current]) {
                    if (data.name.value(childId) == name) {
                        current = childId;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    _this->error(QString("Identifier '%1' not found in current scope").arg(name));
                    return QString(); // 查找失败，返回空字符串
                }
            }

            // 已定位到目标节点，尝试返回其定义位置
            if (data.def.contains(current)) {
                const auto& snippet = data.def[current];
                QString path = data.filePath.value(snippet.file, "<unknown file>");
                return QString("%1:%2:%3")
                    .arg(path)
                    .arg(snippet.beginLine)
                    .arg(snippet.beginColumn);
            }
            // 只有声明没有定义时，可尝试返回声明位置（如果有需要）
            if (data.decl.contains(current)) {
                const auto& snippet = data.decl[current];
                QString path = data.filePath.value(snippet.file, "<unknown file>");
                return QString("decl:%1:%2:%3")
                    .arg(path)
                    .arg(snippet.beginLine)
                    .arg(snippet.beginColumn);
            }
            // 既无定义也无声明
            _this->warn(QString("No definition or declaration found for '%1'").arg(data.name.value(current, "?")));
            return QString();
        });
        getDefOp->argAny();
        operators.insert({ opNames.getDef,std::move(getDefOp) });
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
            QVector<PSCCommandArg> args;
            args.append(PSCCommandArg(true, PSCCommandArg::rawStr, codeLine, ""));
            if (!codeLine.endsWith("\n")) {
                args.append(PSCCommandArg(true, PSCCommandArg::rawStr, "\n", ""));
            }
            common = PSCCommand(
                true, opNames.join, recvNames.outPut, defaultUDI,
                args
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
        QString result = operators[common.op]->call(resolvedArgs,err,context);

        // 将结果写入目标接收器
        context.write(common.recv, common.recvUDI, result);
    }
};