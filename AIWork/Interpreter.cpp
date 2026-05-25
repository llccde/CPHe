#include "Interpreter.h"
#include <QRegularExpression>
#include "CommentTool.h"
#include <QStack>

using namespace awf;

// ---------- TreeNode 定义 ----------
class awf::TreeNode {
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
};

// ---------- Interpreter 实现 ----------
Interpreter::Interpreter(ExceptionCollector& ec) : ec(ec) {}

Interpreter::~Interpreter() {
    delete mRootNode;
}

void Interpreter::riseWarning(const QString& wrn) {
    ec.riseWrn(wrn);
}

void Interpreter::setNodePos(TreeNode* node, int row, int colStart, int length) {
    node->lineBegin = node->lineEnd = row;
    node->colBegin = colStart;
    node->colEnd = colStart + length;
}

M_OperatorType Interpreter::parseOperator(const QString& text, int row, int colStart, TreeNode*& outNode) {
    outNode = new TreeNode(TreeNode::CommandOperator);
    M_OperatorType type = stringToOperatorType(text);
    outNode->opType = type;
    outNode->text = text;
    setNodePos(outNode, row, colStart, text.length());
    return type;
}

M_CommandArg Interpreter::parseArgItem(const QString& itemStr, int row, int colStart, TreeNode*& outNode) {
    outNode = new TreeNode(TreeNode::ArgItem);
    setNodePos(outNode, row, colStart, itemStr.length());

    M_CommandArg arg;
    int eqIdx = itemStr.indexOf('=');
    if (eqIdx > 0) {
        QString key = itemStr.left(eqIdx).trimmed();
        QString val = itemStr.mid(eqIdx + 1).trimmed();
        arg.key = key;
        arg.val = val;
        arg.hasVal = true;

        TreeNode* keyNode = new TreeNode(TreeNode::ArgKey);
        keyNode->text = key;
        setNodePos(keyNode, row, colStart, key.length());
        outNode->children.append(keyNode);
        keyNode->parent = outNode;

        TreeNode* valNode = new TreeNode(TreeNode::ArgVal);
        valNode->text = val;
        setNodePos(valNode, row, colStart + eqIdx + 1, val.length());
        outNode->children.append(valNode);
        valNode->parent = outNode;
    }
    else {
        QString flag = itemStr.trimmed();
        arg.key = flag;
        arg.hasVal = false;

        TreeNode* keyNode = new TreeNode(TreeNode::ArgKey);
        keyNode->text = flag;
        setNodePos(keyNode, row, colStart, flag.length());
        outNode->children.append(keyNode);
        keyNode->parent = outNode;
    }
    return arg;
}

QVector<M_CommandArg> Interpreter::parseArgList(const QString& argPart, int row, int colStart, TreeNode*& outNode) {
    outNode = new TreeNode(TreeNode::ArgList);
    setNodePos(outNode, row, colStart, argPart.length());

    QVector<M_CommandArg> args;
    const QStringList items = argPart.split(',', Qt::SkipEmptyParts);
    int currentCol = colStart;
    for (const QString& item : items) {
        QString trimmed = item.trimmed();
        if (trimmed.isEmpty()) continue;

        int idx = argPart.indexOf(item, currentCol - colStart);
        int itemColStart = (idx >= 0) ? colStart + idx : currentCol;

        TreeNode* itemNode = nullptr;
        M_CommandArg arg = parseArgItem(trimmed, row, itemColStart, itemNode);
        args.append(arg);
        outNode->children.append(itemNode);
        itemNode->parent = outNode;

        currentCol = itemColStart + item.length() + 1;
    }
    return args;
}

QString Interpreter::parseSingleArg(const QString& rest, int row, int colStart, TreeNode*& outNode) {
    outNode = new TreeNode(TreeNode::SingleArg);
    outNode->text = rest;
    setNodePos(outNode, row, colStart, rest.length());
    return rest;
}

bool Interpreter::parseCommandLine(const ParseState& state, M_Command& outCmd, TreeNode*& outNode) {
    outCmd = M_Command();
    QString text = state.commentText.mid(1).trimmed(); // 去 '@'
    if (text.isEmpty()) return false;

    QRegularExpression re("^([A-Za-z_][A-Za-z0-9_]*)");
    auto match = re.match(text);
    if (!match.hasMatch()) return false;

    QString opName = match.captured(1);
    int opColStart = state.commentStartCol + 1; // '@' 之后第一个字符
    TreeNode* opNode = nullptr;
    M_OperatorType opType = parseOperator(opName, state.row, opColStart, opNode);
    outCmd.type = opType;

    QString rest = text.mid(match.capturedLength()).trimmed();
    bool isLong = false;

    auto cmdNode = new TreeNode(TreeNode::Command);
    setNodePos(cmdNode, state.row, state.commentStartCol, state.commentText.length());
    cmdNode->children.append(opNode);
    opNode->parent = cmdNode;

    if (rest.startsWith('{')) {
        isLong = true;
        outCmd.isLongOperator = true;
        rest = rest.mid(1).trimmed();
        int descColStart = opColStart + match.capturedLength() + 1;
        if (!rest.isEmpty()) {
            TreeNode* descNode = nullptr;
            QString desc = parseSingleArg(rest, state.row, descColStart, descNode);
            outCmd.arg = desc;
            cmdNode->children.append(descNode);
            descNode->parent = cmdNode;
        }
        // 长指令作用域将在 loadFile 中创建
    }
    else if (rest.startsWith(',')) {
        rest = rest.mid(1);
        int argColStart = opColStart + match.capturedLength() + 1;
        TreeNode* argListNode = nullptr;
        QVector<M_CommandArg> args = parseArgList(rest, state.row, argColStart, argListNode);
        outCmd.args = args;
        cmdNode->children.append(argListNode);
        argListNode->parent = cmdNode;
    }
    else if (rest.startsWith(':')) {
        rest = rest.mid(1).trimmed();
        int descColStart = opColStart + match.capturedLength() + 1;
        TreeNode* descNode = nullptr;
        QString desc = parseSingleArg(rest, state.row, descColStart, descNode);
        outCmd.arg = desc;
        cmdNode->children.append(descNode);
        descNode->parent = cmdNode;
    }
    else if (!rest.isEmpty()) {
        int descColStart = opColStart + match.capturedLength() + 1;
        TreeNode* descNode = nullptr;
        QString desc = parseSingleArg(rest, state.row, descColStart, descNode);
        outCmd.arg = desc;
        cmdNode->children.append(descNode);
        descNode->parent = cmdNode;
    }

    outNode = cmdNode;
    return true;
}

// ---------- 核心：loadFile ----------
void Interpreter::loadFile(QString filePath) {
    CommentTool tool;
    mLines = tool.analyzeFile(filePath, "//", "/*", "*/");
    mCommandCache.clear();
    mParentRow.clear();
    mInBlockAfterLine.clear();

    mInBlockAfterLine.resize(mLines.size() + 1);
    bool inBlock = false;
    mInBlockAfterLine[0] = inBlock;

    mParentRow.resize(mLines.size());

    delete mRootNode;
    mRootNode = new TreeNode(TreeNode::Root);

    QStack<TreeNode*> scopeStack;
    scopeStack.push(mRootNode);
    QStack<int> longBlockRows;   // 记录当前长指令开始行，用于设置 mParentRow

    for (int i = 0; i < mLines.size(); ++i) {
        const LineInfo& line = mLines[i];

        // 更新多行注释块状态
        if (line.hasMutiLineCommentBegin && !line.hasMutiLineCommentEnd)
            inBlock = true;
        else if (line.hasMutiLineCommentEnd && !line.hasMutiLineCommentBegin)
            inBlock = false;
        mInBlockAfterLine[i + 1] = inBlock;

        // 计算当前行的父行（隶属于哪个长指令）
        int parent = longBlockRows.isEmpty() ? -1 : longBlockRows.top();
        mParentRow[i] = parent;

        // ------------------- 处理非注释行（纯代码） -------------------
        if (!line.isCommentOnly) {
            M_Command cmd;
            cmd.type = MP::notCommand;
            cmd.arg = line.rawLine;
            mCommandCache[i] = cmd;

            // 语法树：在任意作用域下都添加 NaturalText 节点
            auto txtNode = new TreeNode(TreeNode::NaturalText);
            txtNode->text = line.rawLine;
            setNodePos(txtNode, i, 0, line.rawLine.length());
            scopeStack.top()->children.append(txtNode);
            txtNode->parent = scopeStack.top();
            continue;
        }

        // ------------------- 处理注释行 -------------------
        QString commentText = line.commentText.trimmed();

        // 1. 不以 '@' 开头 → 普通注释
        if (!commentText.startsWith('@')) {
            M_Command cmd;
            cmd.type = MP::normalComment;
            cmd.arg = line.rawLine;   // 保留原始行，便于后续查看
            mCommandCache[i] = cmd;

            // 语法树添加 NaturalText（即使不在长指令内，也体现注释）
            auto txtNode = new TreeNode(TreeNode::NaturalText);
            txtNode->text = line.rawLine;
            setNodePos(txtNode, i, 0, line.rawLine.length());
            scopeStack.top()->children.append(txtNode);
            txtNode->parent = scopeStack.top();
            continue;
        }

        // 2. 检查 @} 结束标记
        if (commentText.mid(1).trimmed() == "}") {
            M_Command cmd;
            cmd.type = MP::end;
            mCommandCache[i] = cmd;
            // 不在语法树中为该行生成节点（它是控制标记）

            if (scopeStack.size() > 1 && scopeStack.top()->type == TreeNode::LongCommandScope) {
                TreeNode* closedScope = scopeStack.pop();
                closedScope->lineEnd = i;
                if (!longBlockRows.isEmpty()) longBlockRows.pop();
            }
            continue;
        }

        // 3. 尝试作为指令解析（包括 @msg, @fill, @ref 等）
        ParseState state;
        state.row = i;
        state.originalLine = line.rawLine;
        state.commentText = commentText;
        int firstNonSpace = 0;
        while (firstNonSpace < state.originalLine.length() &&
            state.originalLine[firstNonSpace].isSpace())
            ++firstNonSpace;
        state.commentStartCol = firstNonSpace;

        M_Command cmd;
        TreeNode* cmdNode = nullptr;
        if (parseCommandLine(state, cmd, cmdNode)) {
            // 成功解析为指令
            mCommandCache[i] = cmd;
            mParentRow[i] = parent;  // 更新父行（前面已设，但可保留）

            scopeStack.top()->children.append(cmdNode);
            cmdNode->parent = scopeStack.top();

            // 长指令：创建作用域并压栈
            if (cmd.isLongOperator) {
                auto scopeNode = new TreeNode(TreeNode::LongCommandScope);
                scopeNode->lineBegin = i;
                scopeNode->lineEnd = -1;
                cmdNode->children.append(scopeNode);
                scopeNode->parent = cmdNode;
                scopeStack.push(scopeNode);
                longBlockRows.push(i);
            }
        }
        else {
            // 解析失败（格式错误），降级为普通注释
            M_Command fallback;
            fallback.type = MP::normalComment;
            fallback.arg = line.rawLine;
            mCommandCache[i] = fallback;

            auto txtNode = new TreeNode(TreeNode::NaturalText);
            txtNode->text = line.rawLine;
            setNodePos(txtNode, i, 0, line.rawLine.length());
            scopeStack.top()->children.append(txtNode);
            txtNode->parent = scopeStack.top();
        }
    }

    // 处理文件结束时仍未闭合的长作用域
    while (scopeStack.size() > 1) {
        TreeNode* node = scopeStack.pop();
        if (node->type == TreeNode::LongCommandScope && node->lineEnd == -1)
            node->lineEnd = mLines.size() - 1;
    }
}

// ---------- 从语法树构建缓存（备用或刷新用） ----------
void Interpreter::buildCacheFromTree() {
    if (!mRootNode) return;
    mCommandCache.clear();
    mParentRow.clear();
    mParentRow.resize(mLines.size());
    buildCacheRecursive(mRootNode, mCommandCache, mParentRow, -1);
}

void Interpreter::buildCacheRecursive(TreeNode* node, QHash<int, M_Command>& cache,
    QVector<int>& parentRow, int parentRowIdx) {
    if (!node) return;
    for (TreeNode* child : node->children) {
        if (child->type == TreeNode::Command) {
            // 从子节点重建 M_Command（假设 Command 节点的第一个子节点是 CommandOperator）
            M_Command cmd;
            if (child->children.size() > 0 && child->children[0]->type == TreeNode::CommandOperator) {
                cmd.type = child->children[0]->opType;
                // 收集其他子节点：ArgList / SingleArg / LongCommandScope
                for (int i = 1; i < child->children.size(); ++i) {
                    TreeNode* sub = child->children[i];
                    if (sub->type == TreeNode::SingleArg) {
                        cmd.arg = sub->text;
                    }
                    else if (sub->type == TreeNode::ArgList) {
                        // 从 ArgList 的子节点重建 args
                        for (TreeNode* item : sub->children) {
                            M_CommandArg arg;
                            if (item->type == TreeNode::ArgItem) {
                                // ArgItem 下可能有 ArgKey 和 ArgVal
                                TreeNode* keyNode = nullptr;
                                TreeNode* valNode = nullptr;
                                for (TreeNode* grand : item->children) {
                                    if (grand->type == TreeNode::ArgKey) keyNode = grand;
                                    else if (grand->type == TreeNode::ArgVal) valNode = grand;
                                }
                                if (keyNode) {
                                    arg.key = keyNode->text;
                                    if (valNode) {
                                        arg.val = valNode->text;
                                        arg.hasVal = true;
                                    }
                                    else {
                                        arg.hasVal = false;
                                    }
                                }
                            }
                            cmd.args.append(arg);
                        }
                    }
                    else if (sub->type == TreeNode::LongCommandScope) {
                        // 长指令，标记 isLongOperator
                        cmd.isLongOperator = true;
                    }
                }
            }
            int row = child->lineBegin;
            if (row >= 0 && row < mLines.size()) {
                cache[row] = cmd;
                parentRow[row] = parentRowIdx;
            }

            // 递归处理长指令内的子指令
            if (child->children.size() > 0) {
                // 找到 LongCommandScope 节点
                for (TreeNode* sub : child->children) {
                    if (sub->type == TreeNode::LongCommandScope) {
                        // 该长作用域内的指令的父行是当前指令的行
                        buildCacheRecursive(sub, cache, parentRow, row);
                    }
                }
            }
        }
        else if (child->type == TreeNode::LongCommandScope) {
            // 直接作用域（不应出现在顶层？）
            buildCacheRecursive(child, cache, parentRow, parentRowIdx);
        }
        else {
            // Root, NaturalText 等继续递归
            buildCacheRecursive(child, cache, parentRow, parentRowIdx);
        }
    }
}

// ---------- 原有查询接口 ----------
M_Command Interpreter::getCommandOf(int row) {
    if (mCommandCache.contains(row))
        return mCommandCache[row];
    return M_Command();
}
bool awf::Interpreter::hasCommand(int row)
{
    return mCommandCache.contains(row);
}
bool Interpreter::isCommandComment(int row) {
    if (row < 0 || row >= mLines.size()) return false;
    if (!mLines[row].isCommentOnly) return false;
    QString text = mLines[row].commentText.trimmed();
    return text.startsWith('@');
}

QString Interpreter::getSource(int row) {
    if (row < 0 || row >= mLines.size()) return QString();
    return mLines[row].rawLine;
}

int Interpreter::rowCount() {
    return mLines.size();
}

bool Interpreter::isCommentBlockAfter(int b) const {
    if (b < -1) return false;
    int idx = b + 1;
    if (idx < 0 || idx >= mInBlockAfterLine.size()) return false;
    return mInBlockAfterLine[idx];
}

int Interpreter::getParentRow(int row) const {
    if (row < 0 || row >= mParentRow.size()) return -1;
    return mParentRow[row];
}

QVector<int> Interpreter::getChildRows(int parentRow) const {
    QVector<int> children;
    if (parentRow < 0 || parentRow >= mParentRow.size()) return children;
    for (int i = 0; i < mParentRow.size(); ++i) {
        if (mParentRow[i] == parentRow)
            children.append(i);
    }
    return children;
}