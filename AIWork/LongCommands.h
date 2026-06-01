#pragma once
#include"ExceptionCollector.h"
#include"M_Command.h"
#include"AIClient.h"
#include"qapplication.h"
#include"qclipboard.h"
namespace awf {
    class AIWorkFlow;
    class RuntimeCommand;
    class BasePrompt;
}
// ---------- 符号类型 ----------
enum class SymbolType { Class, Func, Identify };

class awf::RuntimeCommand {
public:
    AIWorkFlow* aiwf = nullptr;
    ExceptionCollector* ec = nullptr;
    M_Command           command;

    virtual ~RuntimeCommand() = default;

    // 进入长指令时调用
    virtual void onLaunch() {}

    // 遇到 @end 时调用
    virtual void onFinish() {}

    // 收到文件内容（来自 @ref 或 @refFile）
    virtual void onReciveFileContent(const QString& content,
        const QString& path,
        Role role = Role::system) {
        if (ec)
            ec->riseErr(QString("不支持在长指令 '@%1' 内使用 %2")
                .arg(operatorTypeToString(command.type))
                .arg("文件引用"));
    }

    // 收到命名要求（@nameFunc / @nameClass）
    virtual void onName(SymbolType type, const QString& name) {
        if (ec)
            ec->riseErr(QString("不支持在长指令 '@%1' 内使用命名功能")
                .arg(operatorTypeToString(command.type)));
    }

    // 收到用户消息（@msg / 普通注释）
    virtual void onMessage(const QString& msg) {
        if (ec)
            ec->riseErr(QString("不支持在长指令 '@%1' 内使用消息")
                .arg(operatorTypeToString(command.type)));
    }

    // 设置模型名称
    virtual void onModuleName(const QString& name) {
        if (ec)
            ec->riseErr(QString("不支持在长指令 '@%1' 内指定模型")
                .arg(operatorTypeToString(command.type)));
    }

    // 将当前 prompt 复制到剪贴板
    virtual void onCopyPrompt() {
        if (ec)
            ec->riseErr(QString("不支持在长指令 '@%1' 内复制 prompt")
                .arg(operatorTypeToString(command.type)));
    }
};

// ---------- 带 prompt 积累的基类 ----------
class awf::BasePrompt : public RuntimeCommand {
protected:
    QVector<ChatMessage> promote;
    QString modelName;
    SymbolType namedType = SymbolType::Identify;
    QString symbolName;

public:
    void onReciveFileContent(const QString& content,
        const QString& path,
        Role role = Role::system) override {
        promote.append({ role, content });
    }

    void onName(SymbolType type, const QString& name) override {
        namedType = type;
        symbolName = name;
        if (type == SymbolType::Func) {
            promote.append({ Role::system,
                QString("用户指定生成的函数必须名为\"%1\",不允许有任何不同").arg(name) });
        }
        else if (type == SymbolType::Class) {
            promote.append({ Role::system,
                QString("用户指定的类名必须为\"%1\",不允许有任何不同").arg(name) });
        }
    }

    void onMessage(const QString& msg) override {
        promote.append({ Role::user, msg });
    }

    void onModuleName(const QString& name) override {
        modelName = name;
    }

    void onCopyPrompt() override {
        QStringList lines;
        for (auto& msg : promote)
            lines << QString("[%1]: %2").arg(roleToString(msg.role), msg.message);
        QApplication::clipboard()->setText(lines.join("\n"));
    }
};
namespace awf {
    // ---------- Fill 指令（生成代码） ----------
    class FillCommand : public BasePrompt {
        int beginRow = -1;
        QString genId;
    public:
        void onLaunch() override;
        void onFinish() override;
    };

    // ---------- Chat 指令（对话） ----------
    class ChatCommand : public BasePrompt {
    public:
        void onFinish() override;
    };

    // ---------- Print 指令（打印 prompt） ----------
    class PrintCommand : public BasePrompt {
    public:
        void onFinish() override;
    };

    // ========== AIWorkFlow 主类 ==========
}
