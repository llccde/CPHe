#pragma once

#include <QString>
#include <QVector>
#include <QHash>
#include <QStack>
#include "ExceptionCollector.h"
#include "qobject.h"
#include "M_Command.h"

namespace awf {

    class TreeNode; // 前向声明

    class Interpreter {
        ExceptionCollector& ec;
    public:
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

        TreeNode* rootNode() const { return mRootNode; }

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

        QVector<LineInfo> mLines;
        mutable QHash<int, M_Command> mCommandCache;
        QVector<bool> mInBlockAfterLine;
        QVector<int> mParentRow;
        TreeNode* mRootNode = nullptr;
    };

} // namespace awf