#pragma once

#include <QObject>
#include <QString>
#include <QMetaEnum>
#include"RuntimeErrcollector.h"
class PSCVarTypeClass : public QObject {
    Q_OBJECT
public:
    enum PSCVarType {
        nullVar,
        rawStr,
        CXCursor,
        FileContentManager,
        codeSnippet,
        stringList
    };
    Q_ENUM(PSCVarType)
    enum PSCSegmRawStrType {
        pInt,
        identifer,//[a-zA-Z_][a-zA-Z0-9_]*
        justStr
    };
    Q_ENUM(PSCSegmRawStrType)
};

using PSCVarType = PSCVarTypeClass::PSCVarType;
using PSCSegmRawStrType = PSCVarTypeClass::PSCSegmRawStrType;
class PSCVar {
protected:
    QString content;
    PSCVarType type;
    PSCSegmRawStrType segm;
public:
    bool checkSegmStrType(RunTimeErrorCollector& err) {
        // 只对 rawStr 类型做段类型检查
        if (type != PSCVarType::rawStr)
            return true;

        switch (segm) {
        case PSCSegmRawStrType::pInt: {
            bool ok = false;
            content.toInt(&ok);
            if (!ok) {
                err.putError("cant convert \"" + content + "\" to int");
                return false;
            }
            return true;
        }
        case PSCSegmRawStrType::identifer: {
            // 标识符规则: [a-zA-Z_][a-zA-Z0-9_]*
            if (content.isEmpty()) {
                err.putError("empty identifier");
                return false;
            }
            QChar first = content.at(0);
            if (!(first.isLetter() || first == '_')) {
                err.putError("identifier must start with a letter or underscore: \"" + content + "\"");
                return false;
            }
            for (int i = 1; i < content.length(); ++i) {
                QChar ch = content.at(i);
                if (!(ch.isLetterOrNumber() || ch == '_')) {
                    err.putError("invalid character in identifier: \"" + content + "\"");
                    return false;
                }
            }
            return true;
        }
        case PSCSegmRawStrType::justStr:
        default:
            // 纯字符串无需检查
            return true;
        }
    }
    void setSegmType(PSCSegmRawStrType t) {
        this->segm = t;
    }
    static PSCVar null() {
        return PSCVar("null", PSCVarType::nullVar);
    }

    bool isNull() const {
        return type == PSCVarType::nullVar;
    }
    PSCVarType getType()const {
        return this->type;
    }
    PSCSegmRawStrType getSegm() const{
        return this->segm;
    }
    PSCVar() : content(""), type(PSCVarType::rawStr) {}

    explicit PSCVar(const QString& content, PSCVarType type = PSCVarType::rawStr)
        : content(content), type(type) {
    }
    explicit PSCVar(const QString& content, PSCSegmRawStrType type,RunTimeErrorCollector* err = nullptr)
        : content(content), type(PSCVarType::rawStr),segm(type) {
        if (err) {
            checkSegmStrType(*err);
        }
        else
        {
            RunTimeErrorCollector err;
            checkSegmStrType(err);
            if (err.hasError()) {
                throw std::runtime_error(err.allToOneString().toStdString());
            }
        }
    }
    QString data()const {
        return content;
    }

    // 基于内容和类型的比较，更符合“详细类型取代 dRef”的语义
    bool operator==(const PSCVar& other) const {
        return content == other.content && type == other.type;
    }
    bool operator!=(const PSCVar& other) const {
        return !(*this == other);
    }

};
