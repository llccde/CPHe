#pragma once
#include<qstring.h>
#include<qvector.h>
class RunTimeErrorCollector {
public:
    struct Msg {
        enum Level { msg, warn, error } level;
        QString content;
    };
    QVector<Msg> msgs;

    void put(const QString& msg) { msgs.append({ Msg::error, msg }); }
    void putError(const QString& msg) { msgs.append({ Msg::error, msg }); }
    void putWarn(const QString& msg) { msgs.append({ Msg::warn, msg }); }
    void putMsg(const QString& msg) { msgs.append({ Msg::msg, msg }); }
    QString allToOneString() const {
        QString result;
        for (const auto& m : msgs) {
            result += QStringLiteral("[%1] %2\n")
                .arg(levelToString(m.level), m.content);
        }
        return result;
    }
    bool hasError() const {
        for (const auto& m : msgs)
            if (m.level == Msg::error) return true;
        return false;
    }

    void output() const {
        for (const auto& e : msgs) {
            switch (e.level) {
            case Msg::error: qDebug() << "Error:" << e.content; break;
            case Msg::warn:  qDebug() << "Warning:" << e.content; break;
            case Msg::msg:   qDebug() << "Info:" << e.content; break;
            default:         qDebug() << "Unknown:" << e.content; break;
            }
        }
    }

    void reset() { msgs.clear(); }
private:
    static QString levelToString(Msg::Level lvl) {
        switch (lvl) {
        case Msg::msg:   return QStringLiteral("msg");
        case Msg::warn:  return QStringLiteral("warn");
        case Msg::error: return QStringLiteral("error");
        }
        return QStringLiteral("unknown");
    }
};