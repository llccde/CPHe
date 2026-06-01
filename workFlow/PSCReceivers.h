#pragma once
#include<qstring.h>
#include<qmap.h>
#include"PSCVar.h"

static const QString defaultUDI = "default";
class PSCReceiver {
protected:
    QMap<QString, PSCVar> data;   // 存储 PSCVar 而非 QString

public:
    PSCReceiver() { receive(defaultUDI, PSCVar("")); }

    virtual void receive(QString userDefID, const PSCVar& result) {
        data.insert(userDefID, result);
    }

    virtual const PSCVar& get(QString userDefID) const {
        static const PSCVar empty;
        auto it = data.find(userDefID);
        return (it != data.end()) ? *it : empty;
    }

    virtual bool has(QString userDefID) const {
        return data.contains(userDefID);
    }

    virtual ~PSCReceiver() = default;
};

class AppendPSCReceiver : public PSCReceiver {
public:
    void receive(QString userDefID, const PSCVar& result) override {
        if (has(userDefID)) {
            // 拼接时需要将两个 PSCVar 转换为字符串再合并
            QString combined = data[userDefID].toString() + result.toString();
            data[userDefID] = PSCVar(combined, PSCVarType::rawStr, nullptr, nullptr);
        }
        else {
            data.insert(userDefID, result);
        }
    }
};

class EmptyPSCReceiver : public PSCReceiver {
public:
    void receive(QString /*userDefID*/, const PSCVar& /*result*/) override {
        // 什么都不做
    }
    bool has(QString /*userDefID*/) const override {
        return true;
    }
    const PSCVar& get(QString /*userDefID*/) const override {
        static const PSCVar empty;
        return empty;
    }
};