// TreeNode.h
#pragma once
#include <QString>
#include <QVector>
#include "M_Command.h"
namespace awf {
    class TreeNode {
    public:
        enum Type {
            Root,
            Command,
            LongCommandScope,
            CommandOperator,
            SingleArg,
            ArgList,
            ArgItem,
            ArgKey,
            ArgVal,
            NaturalText,
            Error
        };
        Type type;
        int lineBegin, lineEnd;
        int colBegin, colEnd;
        TreeNode* parent = nullptr;
        QVector<TreeNode*> children;
        M_OperatorType opType = M_OperatorType::notCommand;
        QString text;
        TreeNode(Type t) : type(t) {}
        ~TreeNode() { qDeleteAll(children); }
        /**
         * 获取包含指定行列位置的最深节点
         * @param line 行号
         * @param col  列号
         * @return 最深的包含该位置的节点；若当前节点不包含该位置则返回 nullptr
         */
        TreeNode* deepestNodeAt(int line, int col) {
            // 检查当前节点是否包含 (line, col)
            if (!type == Root) {
                if (line < lineBegin || line > lineEnd)
                    return nullptr;
                if (line == lineBegin && col < colBegin)
                    return nullptr;
                if (line == lineEnd && col > colEnd)
                    return nullptr;
            }

            // 在子节点中递归查找更深的节点
            for (auto child : children) {
                TreeNode* found = child->deepestNodeAt(line, col);
                if (found)
                    return found;
            }
            // 没有子节点包含该位置，当前节点即为最深节点
            return this;
        }
    };
} // namespace awf