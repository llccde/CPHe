#pragma once
#include <QVector>
#include "PSCVar.h"          // 假设已有 PSCVar 定义
#include"PSCOperators.h"
#include "FileContentManager.h"
PSCVar builtin_join(const QVector<PSCVar>& args, PSCOperator* op);
PSCVar builtin_printOut(const QVector<PSCVar>& args, PSCOperator* op);
PSCVar builtin_setFile(const QVector<PSCVar>& args, PSCOperator* op);
PSCVar builtin_getDef(const QVector<PSCVar>& args, PSCOperator* op);
PSCVar builtin_getDefDepth(const QVector<PSCVar>& args, PSCOperator* op);
FileContentManager getDefDepth_impl(CXCursor cx,int dep, PSCOperator* op);