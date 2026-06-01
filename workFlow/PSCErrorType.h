// PSCErrorType.h
#ifndef PSCERRORTYPE_H
#define PSCERRORTYPE_H
#include"qobject.h"
class PSCErrorType :public QObject{
    Q_OBJECT
public:
    enum errorType {
        InvalidOp,
        InvalidRecv,
        MissingCloseParen,
        MissingColon,
        InvalidCall,
        TrailingChars
        // ... 其他枚举值
    };
    Q_ENUM(errorType);
};
#endif // PSCERRORTYPE_H