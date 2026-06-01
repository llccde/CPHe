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
            Error,
            Invalid   // 新增：表示无效节点
        };

        Type type;
        int lineBegin, lineEnd;
        int colBegin, colEnd;
        TreeNode* parent = nullptr;
        QVector<TreeNode*> children;
        M_OperatorType opType = M_OperatorType::notCommand;
        QString text;
        bool isValid = true;   // 新增：标识节点是否有效

        TreeNode(Type t) : type(t) {}

        ~TreeNode() { qDeleteAll(children); }

        // 获取全局无效节点（单例）
        static TreeNode* invalidNode() {
            static TreeNode node(Invalid);
            node.isValid = false;
            return &node;
        }

        // 向上查找第一个指定类型的父节点
        TreeNode* getParentOfType(Type t) {
            TreeNode* cur = parent;
            while (cur) {
                if (cur->type == t)
                    return cur;
                cur = cur->parent;
            }
            return invalidNode();   // 未找到返回无效节点
        }

        // 返回第一个指定类型的直接子节点
        TreeNode* getChildOfType(Type t) {
            for (auto* child : children) {
                if (child->type == t)
                    return child;
            }
            return invalidNode();   // 未找到返回无效节点
        }

        // 返回所有指定类型的直接子节点
        QVector<TreeNode*> getChildrenOfType(Type t) {
            QVector<TreeNode*> result;
            for (auto* child : children) {
                if (child->type == t)
                    result.append(child);
            }
            if (result.isEmpty()) {
                result.append(invalidNode());  // 约定：找不到也返回一个包含无效节点的向量
            }
            return result;
        }

        /**
         * 获取包含指定行列位置的最深节点
         */
        TreeNode* deepestNodeAt(int line, int col) {
            // 修正原逻辑错误：应为 type != Root
            if (type != Root) {
                if (line < lineBegin || line > lineEnd)
                    return nullptr;
                if (line == lineBegin && col < colBegin)
                    return nullptr;
                if (line == lineEnd && col > colEnd)
                    return nullptr;
            }

            for (auto* child : children) {
                TreeNode* found = child->deepestNodeAt(line, col);
                if (found)
                    return found;
            }
            return this;
        }
    };

} // namespace awf