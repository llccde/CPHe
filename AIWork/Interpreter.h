#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <QStack>
#include "ExceptionCollector.h"
#include "qobject.h"
#include "M_Command.h"
#include<memory>
namespace awf {

    class TreeNode; // 前向声明

    class Interpreter {
        ExceptionCollector& ec;
    public:
        TreeNode* rootNode() const { return mRootNode.get(); }
        const QVector<LineInfo>& lines() const { return mLines; }
        Interpreter(ExceptionCollector& ec);
        ~Interpreter();

        void riseWarning(const QString& wrn);
        void loadFile(QString filePath);
        bool hasCommand(int row);
        M_Command getCommandOf(int row);
        bool isCommandComment(int row);
        QString getSource(int row);
        int rowCount();

        bool isCommentBlockAfter(int b) const;
        int getParentRow(int row) const;
        QVector<int> getChildRows(int parentRow) const;
        int getHierarchyLevel(int row) const {
            if (row < 0 || row >= mHierarchyLevels.size()) return -1;
            return mHierarchyLevels[row];
        }

        int getParentCommand(int row) const {
            int level = getHierarchyLevel(row);
            if (level <= 0) return -1;
            // 向上找第一个层级为 level-1 的行
            for (int i = row - 1; i >= 0; --i) {
                if (getHierarchyLevel(i) == level - 1) return i;
            }
            return -1;
        }

    private:
        struct ParseState {
            int row;
            QString originalLine;
            QString commentText;
            int commentStartCol;  // commentText 在 originalLine 中的起始列（0-based）
        };

        // 递归下降解析核心
        bool parseCommandLine(const ParseState& state, M_Command& outCmd, TreeNode*& outNode);
        M_OperatorType parseOperator(const QString& text, int row, int colStart, TreeNode*& outNode);
        QVector<M_CommandArg> parseArgList(const QString& argPart, int row, int colStart, TreeNode*& outNode);
        M_CommandArg parseArgItem(const QString& itemStr, int row, int colStart, TreeNode*& outNode);
        QString parseSingleArg(const QString& rest, int row, int colStart, TreeNode*& outNode);

        void setNodePos(TreeNode* node, int row, int colStart, int length);

        // 从语法树构建缓存和父子行数组
        void buildCacheFromTree();
        void buildCacheRecursive(TreeNode* node, QHash<int, M_Command>& cache,
            QVector<int>& parentRow, int parentRowIdx);
        void computeHierarchyLevels();
        QVector<LineInfo> mLines;
        mutable QHash<int, M_Command> mCommandCache;
        QVector<bool> mInBlockAfterLine;
        QVector<int> mParentRow;
        std::unique_ptr<TreeNode> mRootNode = nullptr;
        QVector<int> mHierarchyLevels;
    };

} // namespace awf