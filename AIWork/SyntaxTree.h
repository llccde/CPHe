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
            Command,               // 一条完整指令行（单行或长指令头）
            LongCommandScope,      // 长指令作用域（@xxx{ ... @end 的全部内容）
            CommandOperator,       // 操作符标识符，如 "fill"
            SingleArg,             // 单一自然语言参数
            ArgList,               // 参数列表整体（包含多个 ArgItem）
            ArgItem,               // 一个参数项（可能有关键字和值）
            ArgKey,                // 参数键
            ArgVal,                // 参数值
            NaturalText,           // 长指令作用域内的自由文本行（非子指令）
            Error                  // 解析错误
        };

        Type type;
        int lineBegin, lineEnd;     // 行范围（包含），对于单行节点两者相同
        int colBegin, colEnd;       // 列范围（在 lineBegin 行内，半开区间 [colBegin, colEnd) ）
        TreeNode* parent = nullptr;
        QVector<TreeNode*> children;

        // 语义负载
        M_OperatorType opType = M_OperatorType::notCommand;  // 对于 CommandOperator 有效
        QString text;               // 原始字符串或关键值（如操作符名、参数键值、描述文本）

        TreeNode(Type t) : type(t) {}
        ~TreeNode() { qDeleteAll(children); }

        // 便捷方法
        bool isLeaf() const { return children.isEmpty(); }
        TreeNode* childAt(int i) const {
            return (i >= 0 && i < children.size()) ? children[i] : nullptr;
        }
        TreeNode* childOfPos(int row, int column) {
            // 判断当前节点是否包含指定位置
            auto containsPosition = [this](int r, int c) -> bool {
                if (r < lineBegin || r > lineEnd) return false;
                if (r == lineBegin) {
                    if (c < colBegin) return false;
                    // 单行节点严格遵循半开区间 [colBegin, colEnd)
                    if (lineEnd == lineBegin && c >= colEnd) return false;
                }
                // 多行节点：首行从 colBegin 开始到行尾，中间行全包含，末行整行包含
                return true;
                };

            // 递归查找叶子
            std::function<TreeNode* (TreeNode*, int, int)> findLeafAt;
            findLeafAt = [&](TreeNode* node, int r, int c) -> TreeNode* {
                if (!node || !containsPosition(r, c)) return nullptr;
                if (node->isLeaf()) return node;
                for (TreeNode* child : node->children) {
                    TreeNode* found = findLeafAt(child, r, c);
                    if (found) return found;
                }
                // 当前节点虽包含位置但不是叶子，也没有匹配的子节点 -> 未找到叶子
                return nullptr;
                };

            // 先用原始列寻找
            TreeNode* result = findLeafAt(this, row, column);
            if (result) return result;

            // 未找到则列号减 1 再试（向左移动一格）
            if (column > 0) {
                result = findLeafAt(this, row, column - 1);
            }
            return result;
        }
    };

} // namespace awf