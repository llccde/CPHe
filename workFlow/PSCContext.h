#pragma once
#include<qmap.h>
#include"qstring.h"
#include"CppFileAnalyzer.h"
#include"PSCReceivers.h"
#include"FileContentManager.h"
class RunTimeErrorCollector;
template<class T>
inline QString sizeAsKey(T sizeAble) {
    return QString::number(sizeAble.size());
};

class PSCContext {
    unsigned long long idCount = 0;
public:
    QString getID() { return QString::number(idCount++); }
    struct CPPContext {
        QMap<QString, CXCursor> savedCursor;
        QMap<QString, CppCodeAnalyzerResult::CodeSnippet> savedSnippet;
        QMap<QString, QString> savedSnippetFilePath;
        QString MainFilePath;
        QString workingFolder;
        CppCodeAnalyzer analyzer;
        CppCodeAnalyzerResult result;
    } cppContext;
    QMap<QString, QVector<QString>> savedStringList;
    std::map<QString, std::unique_ptr<PSCReceiver>> receivers;

    // 返回 PSCVar 而非 QString
    PSCVar get(QString receiverName, QString userDefID) {
        
        auto it = receivers.find(receiverName);
        if (it != receivers.end()) {
            return it->second->get(userDefID);
        }
        return PSCVar();
    }

    void write(QString receiverName, QString userDefID, const PSCVar& value) {
        auto it = receivers.find(receiverName);
        if (it != receivers.end()) {
            it->second->receive(userDefID, value);
        }
    }

    void registe(std::unique_ptr<PSCReceiver> recv, QString name) {
        if (recv && receivers.find(name) == receivers.end()) {
            receivers[name] = std::move(recv);
        }
    }

    bool contains(QString name) const {
        return receivers.find(name) != receivers.end();
    }

    PSCReceiver* getReceiver(QString name) {
        auto it = receivers.find(name);
        return (it != receivers.end()) ? it->second.get() : nullptr;
    }
    RunTimeErrorCollector* collector;
};