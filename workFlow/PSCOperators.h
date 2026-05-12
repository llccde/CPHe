#pragma once

#include <functional>
#include <memory>
#include <QString>
#include <QVector>
#include <QMetaEnum>

#include "PSCVar.h"
#include "RuntimeErrcollector.h"
#include "PSCContext.h"

class PSCOperator {
protected:
    // 包装类型：支持普通 PSCVarType 以及 rawStr 的细分类型
    class WarpPscVarType {
    public:
        PSCVarType type;
        PSCSegmRawStrType segmStr = PSCSegmRawStrType::justStr;

        WarpPscVarType(PSCVarType t);
        WarpPscVarType(PSCSegmRawStrType se);
        WarpPscVarType(PSCVarType t, PSCSegmRawStrType se);
    };

    enum ArgNumType { any, atLeast, noMoreThan, onlyExactNum } argType = any;
    int theExactNum = -1;
    QVector<WarpPscVarType> argTypes;
    QString name;

    bool assertReturnType = false;
    bool returnTypeCanBeNull = true;
    PSCVarType returnType = PSCVarType::nullVar;

    virtual PSCVar call_impl(QVector<PSCVar>& args) = 0;
    virtual bool checkArg_child(QVector<PSCVar>& /*args*/) { return true; }

    explicit PSCOperator(const QString& name);

    static QString typeName(PSCVarType t);
    static QString segmTypeName(PSCSegmRawStrType st);
    static QString warpTypeName(const WarpPscVarType& w);
    static bool typeMatches(const PSCVar& var, const WarpPscVarType& expected);

public:
    RunTimeErrorCollector* collector = nullptr;
    PSCContext* context = nullptr;

    PSCOperator() = default;
    virtual ~PSCOperator();

    // 设置参数数量约束（内联实现）
    void argAny() { argType = any; argTypes.clear(); }
    void argLeast(int leastNum) { argType = atLeast; theExactNum = leastNum; argTypes.clear(); }
    void argNoMore(int maxNum) { argType = noMoreThan; theExactNum = maxNum; argTypes.clear(); }
    void argExact(int thNum) { argType = onlyExactNum; theExactNum = thNum; argTypes.clear(); }

    void setArgTypes(const QVector<WarpPscVarType>& types);
    void setArgTypes(const QVector<PSCVarType>& types);

    void setReturnType(PSCVarType t, bool canBeNull = true);

    PSCVar call(QVector<PSCVar>& args, RunTimeErrorCollector& err, PSCContext& ctx);

    void error(const QString& msg);
    void warn(const QString& wrn);
    void message(const QString& msg);

    QString redCtx(const QString& recv, const QString& udi);
    void setName(QString s) { name = s; }

private:
    bool checkArg(QVector<PSCVar>& args);
    bool checkArgTypes(const QVector<PSCVar>& args);
};

class PSCLambdaOperator : public PSCOperator {
public:
    using Callable = std::function<PSCVar(const QVector<PSCVar>& args, PSCOperator* _this)>;

    explicit PSCLambdaOperator(Callable ca, const QString& name = "lambdaOp");
    ~PSCLambdaOperator() override;

    PSCVar call_impl(QVector<PSCVar>& args) override;

    static std::unique_ptr<PSCLambdaOperator> getByLambda(Callable lambda);

private:
    Callable callback;
};