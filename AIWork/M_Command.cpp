#include"M_Command.h"
using namespace awf;
QString M_Command::toString() const {
    QStringList parts;
    parts << QString("M_Command(type=%1").arg(operatorTypeToString(type));

    if (!arg.isEmpty())
        parts << QString("arg=\"%1\"").arg(arg);

    if (!args.isEmpty()) {
        QStringList pairs;
        for (auto& i : args)
            if(i.hasVal)
            {
                pairs << QString("%1=%2").arg(i.key, i.val);
            }
            else
            {
                pairs.append(i.key);
            }
        parts << QString("args=[%1]").arg(pairs.join(", "));
    }

    if (isLongOperator)
        parts << "isLongOperator=true";

    parts << ")";
    return parts.join(", ");
}
QString awf::operatorTypeToString(M_OperatorType type) {
    const QMetaEnum meta = QMetaEnum::fromType<M_OperatorTypeClass::M_OperatorTypeInner>();
    const char* key = meta.valueToKey(static_cast<int>(type));
    return key ? QString::fromLatin1(key) : QString();
}
M_OperatorType awf::stringToOperatorType(const QString& opStr){
    const QMetaEnum meta = QMetaEnum::fromType<M_OperatorTypeClass::M_OperatorTypeInner>();
    bool ok = false;
    int val = meta.keyToValue(opStr.toLatin1().constData(), &ok);
    if (ok) {
        return static_cast<M_OperatorType>(val);
    }
    // 未知字符串可返回特殊值或抛出异常，这里返回 notCommand 作为默认
    return M_OperatorType::notCommand;
}

const QString& M_Command::getArg(const QString& key) {
    static const QString empty;
    for (auto& i : args) {
        if (i.key == key) {
            return i.val;
        };
    }
    return empty;
}

bool awf::M_Command::contains(const QString& key)
{
    for (auto&i:args)
    {
        if (i.key == key) {
            return true;
        }

    }
    return false;
}
// M_Command.cpp
const QString& M_Command::getArg(ArgsClass::Arg argType)
{
    return getArg(ArgsClass::toString(argType));   // 委托给字符串版本
}

bool M_Command::contains(ArgsClass::Arg argType)
{
    return contains(ArgsClass::toString(argType));
}