// PSCLangErr.h
#ifndef PSCLANGERR_H
#define PSCLANGERR_H

#include "PSCErrorType.h"

struct PSCLangErr {
    int begin;           // 错误在原始字符串中的起始索引（包含）
    int end;             // 错误在原始字符串中的结束索引（不包含）
    PSCErrorType::errorType type;
};

#endif // PSCLANGERR_H