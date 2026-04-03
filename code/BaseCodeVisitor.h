// CodeAnalyzer.h 或单独的头文件
#pragma once
#include <clang-c/Index.h>
class NameMapNode;
using NameMap = NameMapNode;

class BaseVisitor {
public:
    virtual ~BaseVisitor() = default;

    // 纯虚函数：遍历 cursor 的回调
    virtual CXChildVisitResult cursorVisitor(CXCursor cursor, CXCursor parent) = 0;

    // 纯虚函数：获取构建好的名称树
    virtual NameMap* getNameMap() = 0;
};