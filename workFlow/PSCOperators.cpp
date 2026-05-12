#include "PSCOperators.h"

#include <QDebug>

PSCOperator::WarpPscVarType::WarpPscVarType(PSCVarType t)
    : type(t) {
}

PSCOperator::WarpPscVarType::WarpPscVarType(PSCSegmRawStrType se)
    : type(PSCVarType::rawStr), segmStr(se) {
}

PSCOperator::WarpPscVarType::WarpPscVarType(PSCVarType t, PSCSegmRawStrType se)
    : type(t), segmStr(se) {
}
QString PSCOperator::typeName(PSCVarType t) {
    return QMetaEnum::fromType<PSCVarTypeClass::PSCVarType>().valueToKey(t);
}

QString PSCOperator::segmTypeName(PSCSegmRawStrType st) {
    return QMetaEnum::fromType<PSCVarTypeClass::PSCSegmRawStrType>().valueToKey(st);
}

QString PSCOperator::warpTypeName(const WarpPscVarType& w) {
    QString base = typeName(w.type);
    if (w.type == PSCVarType::rawStr) {
        base += QStringLiteral("(") + segmTypeName(w.segmStr) + QStringLiteral(")");
    }
    return base;
}

bool PSCOperator::typeMatches(const PSCVar& var, const WarpPscVarType& expected) {
    if (var.getType() != expected.type)
        return false;
    if (expected.type == PSCVarType::rawStr)
        return var.getSegm() == expected.segmStr;
    return true;
}
PSCOperator::PSCOperator(const QString& name) : name(name) {}

PSCOperator::~PSCOperator() = default;
void PSCOperator::setArgTypes(const QVector<WarpPscVarType>& types) {
    argTypes = types;
    if (!types.isEmpty()) {
        if (argType == any && types.size() != 1) {
            warn("setArgTypes: for 'any', only one type will be used (first)");
            argTypes = types.mid(0, 1);
        }
        else if (argType == atLeast && types.size() < theExactNum) {
            warn(QString("setArgTypes: for 'atLeast %1', at least %1 types required")
                .arg(theExactNum));
        }
    }
}

void PSCOperator::setArgTypes(const QVector<PSCVarType>& types) {
    QVector<WarpPscVarType> warped;
    warped.reserve(types.size());
    for (PSCVarType t : types)
        warped.append(WarpPscVarType(t));
    setArgTypes(warped);
}

void PSCOperator::setReturnType(PSCVarType t, bool canBeNull) {
    assertReturnType = true;
    returnType = t;
    returnTypeCanBeNull = canBeNull;
}
bool PSCOperator::checkArg(QVector<PSCVar>& args) {
    if (!checkArg_child(args))
        return false;

    for (int i = 0; i < args.size(); ++i) {
        if (args[i].isNull()) {
            error(QString("Argument %1 is null").arg(i + 1));
            return false;
        }
    }
    switch (argType) {
    case any:
        break;
    case atLeast:
        if (args.size() < theExactNum) {
            error(QString("At least %1 arguments required, but %2 provided")
                .arg(theExactNum).arg(args.size()));
            return false;
        }
        break;
    case noMoreThan:
        if (args.size() > theExactNum) {
            error(QString("No more than %1 arguments allowed, but %2 provided")
                .arg(theExactNum).arg(args.size()));
            return false;
        }
        break;
    case onlyExactNum:
        if (args.size() != theExactNum) {
            error(QString("Exactly %1 arguments required, but %2 provided")
                .arg(theExactNum).arg(args.size()));
            return false;
        }
        break;
    }

    return checkArgTypes(args);
}

bool PSCOperator::checkArgTypes(const QVector<PSCVar>& args) {
    if (argTypes.isEmpty())
        return true;

    switch (argType) {
    case any: {
        if (argTypes.size() != 1) {
            error("Internal: ArgTypes for 'any' must have exactly one type");
            return false;
        }
        const WarpPscVarType& expected = argTypes.first();
        for (int i = 0; i < args.size(); ++i) {
            if (!typeMatches(args[i], expected)) {
                error(QString("Argument %1 type mismatch: expected %2, got %3")
                    .arg(i + 1)
                    .arg(warpTypeName(expected))
                    .arg(warpTypeName(WarpPscVarType(args[i].getType(), args[i].getSegm()))));
                return false;
            }
        }
        break;
    }
    case atLeast: {
        if (argTypes.size() < theExactNum) {
            error(QString("Internal: ArgTypes for 'atLeast' must have at least %1 types")
                .arg(theExactNum));
            return false;
        }
        for (int i = 0; i < theExactNum && i < args.size(); ++i) {
            if (!typeMatches(args[i], argTypes[i])) {
                error(QString("Argument %1 type mismatch: expected %2, got %3")
                    .arg(i + 1)
                    .arg(warpTypeName(argTypes[i]))
                    .arg(warpTypeName(WarpPscVarType(args[i].getType(), args[i].getSegm()))));
                return false;
            }
        }
        if (args.size() > theExactNum) {
            const WarpPscVarType& lastType = argTypes.last();
            for (int i = theExactNum; i < args.size(); ++i) {
                if (!typeMatches(args[i], lastType)) {
                    error(QString("Argument %1 type mismatch: expected %2, got %3")
                        .arg(i + 1)
                        .arg(warpTypeName(lastType))
                        .arg(warpTypeName(WarpPscVarType(args[i].getType(), args[i].getSegm()))));
                    return false;
                }
            }
        }
        break;
    }
    default:
        // noMoreThan 和 onlyExactNum 的情况：按位置一一对应
        if (args.size() != argTypes.size())
            break;  // 已在数量检查中报错，这里静默
        for (int i = 0; i < args.size(); ++i) {
            if (!typeMatches(args[i], argTypes[i])) {
                error(QString("Argument %1 type mismatch: expected %2, got %3")
                    .arg(i + 1)
                    .arg(warpTypeName(argTypes[i]))
                    .arg(warpTypeName(WarpPscVarType(args[i].getType(), args[i].getSegm()))));
                return false;
            }
        }
        break;
    }
    return true;
}

PSCVar PSCOperator::call(QVector<PSCVar>& args, RunTimeErrorCollector& err, PSCContext& ctx) {
    collector = &err;
    context = &ctx;

    if (!checkArg(args))
        return PSCVar();

    if (!assertReturnType)
        return call_impl(args);

    auto res = call_impl(args);
    if (!returnTypeCanBeNull) {
        if (res.isNull()) {
            error("Return value is null but null is not allowed for this operator.");
            return PSCVar();
        }
    }
    if (res.getType() != returnType) {
        error(QString("Return type mismatch: expected %1, got %2")
            .arg(typeName(returnType))
            .arg(typeName(res.getType())));
        return PSCVar();
    }
    return res;
}

void PSCOperator::error(const QString& msg) {
    if (collector)
        collector->putError("in operator " + name + " : " + msg);
}

void PSCOperator::warn(const QString& wrn) {
    if (collector)
        collector->putWarn("in operator " + name + " : " + wrn);
}

void PSCOperator::message(const QString& msg) {
    if (collector)
        collector->putMsg("in operator " + name + " : " + msg);
}

QString PSCOperator::redCtx(const QString& recv, const QString& udi) {
    if (context && context->contains(recv)) {
        auto* receiver = context->getReceiver(recv);
        if (receiver && receiver->has(udi))
            return context->get(recv, udi).data();
    }
    return {};
}

PSCLambdaOperator::PSCLambdaOperator(Callable ca, const QString& name)
    : PSCOperator(name), callback(std::move(ca)) {
}

PSCLambdaOperator::~PSCLambdaOperator() = default;

PSCVar PSCLambdaOperator::call_impl(QVector<PSCVar>& args) {
    if (callback)
        return callback(args, this);
    return PSCVar();
}

std::unique_ptr<PSCLambdaOperator> PSCLambdaOperator::getByLambda(Callable lambda) {
    return std::make_unique<PSCLambdaOperator>(std::move(lambda));
}