#include"M_Command.h"
using namespace awf;
QString M_Command::toString() const {
    QStringList parts;
    parts << QString("M_Command(type=%1").arg(operatorTypeToString(type));

    if (!arg.isEmpty())
        parts << QString("arg=\"%1\"").arg(arg);

    if (!args.isEmpty()) {
        QStringList pairs;
        for (auto it = args.cbegin(); it != args.cend(); ++it)
            pairs << QString("%1=%2").arg(it.key(), it.value());
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
    auto it = args.find(key);
    return (it != args.end()) ? it.value() : empty;
}