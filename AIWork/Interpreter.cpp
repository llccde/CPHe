#include "Interpreter.h"
#include <QRegularExpression>
#include "CommentTool.h"
#include <QStack>
#include"SyntaxTree.h"
using namespace awf;

Interpreter::Interpreter(ExceptionCollector& ec) : ec(ec) {}

Interpreter::~Interpreter() {
}

void Interpreter::riseWarning(const QString& wrn) {
    ec.Wrn(wrn);
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
    auto cmdNode = new TreeNode(TreeNode::Command);
    setNodePos(cmdNode, state.row, state.commentStartCol, state.commentText.length());
    outNode = cmdNode;
    QString text = state.commentText.mid(1).trimmed(); // 去 '@'
    if (text.isEmpty()) return false;

    // 特殊处理：@} 会作为 end 指令被 loadFile 直接处理，不进入此处
    QRegularExpression re("^([A-Za-z_][A-Za-z0-9_]*)");
    auto match = re.match(text);
    if (!match.hasMatch()) return false;

    QString opName = match.captured(1);
    int opColStart = state.commentStartCol + 1; // '@' 之后第一个字符
    TreeNode* opNode = nullptr;
    M_OperatorType opType = parseOperator(opName, state.row, opColStart, opNode);
    outCmd.type = opType;

    QString rest = text.mid(match.capturedLength()).trimmed();
    cmdNode->children.append(opNode);
    opNode->parent = cmdNode;

    if (rest.startsWith('{')) {
        // 长指令标记：只影响 isLongOperator 属性，不创建作用域
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
        // 不再创建 LongCommandScope 节点
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

    
    return true;
}

// ---------- 核心：loadFile（已移除 LongCommandScope）----------
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

    mRootNode.reset(new TreeNode(TreeNode::Root));

    for (int i = 0; i < mLines.size(); ++i) {
        const LineInfo& line = mLines[i];

        // 更新多行注释块状态
        if (line.hasMutiLineCommentBegin && !line.hasMutiLineCommentEnd)
            inBlock = true;
        else if (line.hasMutiLineCommentEnd && !line.hasMutiLineCommentBegin)
            inBlock = false;
        mInBlockAfterLine[i + 1] = inBlock;

        // 所有指令平级，无父子关系
        mParentRow[i] = -1;

        // ------------------- 处理非注释行（纯代码） -------------------
        if (!line.isCommentOnly) {
            M_Command cmd;
            cmd.type = MP::notCommand;
            cmd.arg = line.rawLine;
            mCommandCache[i] = cmd;

            auto txtNode = new TreeNode(TreeNode::NaturalText);
            txtNode->text = line.rawLine;
            setNodePos(txtNode, i, 0, line.rawLine.length());
            mRootNode->children.append(txtNode);
            txtNode->parent = mRootNode.get();
            continue;
        }

        // ------------------- 处理注释行 -------------------
        QString commentText = line.commentText.trimmed();

        // 1. 不以 '@' 开头 → 普通注释
        if (!commentText.startsWith('@')) {
            M_Command cmd;
            cmd.type = MP::normalComment;
            cmd.arg = line.rawLine;
            mCommandCache[i] = cmd;

            auto txtNode = new TreeNode(TreeNode::NaturalText);
            txtNode->text = line.rawLine;
            setNodePos(txtNode, i, 0, line.rawLine.length());
            mRootNode->children.append(txtNode);
            txtNode->parent = mRootNode.get();
            continue;
        }

        // 2. 检查 @} ：现在作为普通 end 指令
        if (commentText.mid(1).trimmed() == "}") {
            M_Command cmd;
            cmd.type = MP::end;
            mCommandCache[i] = cmd;

            // 创建 end 指令的语法树节点（Command 节点）
            auto cmdNode = new TreeNode(TreeNode::Command);
            // 定位：从行首到注释结束
            int firstNonSpace = 0;
            while (firstNonSpace < line.rawLine.length() &&
                line.rawLine[firstNonSpace].isSpace())
                ++firstNonSpace;
            setNodePos(cmdNode, i, firstNonSpace, line.commentText.length());

            // 操作符节点 "@}" 或 "}" ？这里用 "}" 表示操作符文本
            auto opNode = new TreeNode(TreeNode::CommandOperator);
            opNode->opType = MP::end;
            opNode->text = "}";
            setNodePos(opNode, i, firstNonSpace + 1, 1); // '@' 后的 '}'
            cmdNode->children.append(opNode);
            opNode->parent = cmdNode;

            mRootNode->children.append(cmdNode);
            cmdNode->parent = mRootNode.get();
            continue;
        }

        // 3. 尝试作为指令解析（@msg, @fill, @ref 等）
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
            mRootNode->children.append(cmdNode);
            cmdNode->parent = mRootNode.get();
        }
        else {
            mRootNode->children.append(cmdNode);
            cmdNode->parent = mRootNode.get();
            //// 解析失败（格式错误），降级为普通注释
            //M_Command fallback;
            //fallback.type = MP::normalComment;
            //fallback.arg = line.rawLine;
            //mCommandCache[i] = fallback;

            //auto txtNode = new TreeNode(TreeNode::NaturalText);
            //txtNode->text = line.rawLine;
            //setNodePos(txtNode, i, 0, line.rawLine.length());
            //mRootNode->children.append(txtNode);
            //txtNode->parent = mRootNode.get();
        }
    }
}

// ---------- 从语法树构建缓存（备用）----------
void Interpreter::buildCacheFromTree() {
    if (!mRootNode) return;
    mCommandCache.clear();
    mParentRow.clear();
    mParentRow.resize(mLines.size());
    buildCacheRecursive(mRootNode.get(), mCommandCache, mParentRow, -1);
}

void Interpreter::buildCacheRecursive(TreeNode* node, QHash<int, M_Command>& cache,
    QVector<int>& parentRow, int parentRowIdx) {
    if (!node) return;
    for (TreeNode* child : node->children) {
        if (child->type == TreeNode::Command) {
            M_Command cmd;
            if (child->children.size() > 0 && child->children[0]->type == TreeNode::CommandOperator) {
                cmd.type = child->children[0]->opType;
                for (int i = 1; i < child->children.size(); ++i) {
                    TreeNode* sub = child->children[i];
                    if (sub->type == TreeNode::SingleArg) {
                        cmd.arg = sub->text;
                    }
                    else if (sub->type == TreeNode::ArgList) {
                        for (TreeNode* item : sub->children) {
                            M_CommandArg arg;
                            if (item->type == TreeNode::ArgItem) {
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
                    // 忽略已经不存在的 LongCommandScope 节点
                }
            }
            int row = child->lineBegin;
            if (row >= 0 && row < mLines.size()) {
                cache[row] = cmd;
                parentRow[row] = parentRowIdx; // 始终 -1
            }
            // 不再需要递归作用域，所有指令已平级
        }
        else {
            buildCacheRecursive(child, cache, parentRow, parentRowIdx);
        }
    }
}

void Interpreter::computeHierarchyLevels() {
    mHierarchyLevels.resize(mLines.size());
    int currentLevel = 0;

    for (int i = 0; i < mLines.size(); ++i) {
        M_Command cmd;
        bool hasCmd = mCommandCache.contains(i);
        if (hasCmd) cmd = mCommandCache[i];

        if (hasCmd && cmd.type == MP::end) {
            // @} 行本身属于外一层级，先降级再记录
            if (currentLevel > 0) {
                --currentLevel;
            }
            else {
                riseWarning(QString("Unmatched @} at line %1").arg(i));
            }
            mHierarchyLevels[i] = currentLevel;
        }
        else {
            // 普通行：记录当前层级
            mHierarchyLevels[i] = currentLevel;
            // longOp 开启下一级
            if (hasCmd && cmd.isLongOperator) {
                ++currentLevel;
            }
        }
    }

    // 文件结束时存在未闭合的 longOp
    if (currentLevel > 0) {
        riseWarning(QString("%1 unclosed long command(s) at end of file").arg(currentLevel));
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
int Interpreter::getCommentStartOfCommentRow(int row) const {
    if (!isCommentBlockAfter(row)) {
        return -1;
    }

    for (int i = row; i >= 0; --i) {
        if (!mInBlockAfterLine[i] && mInBlockAfterLine[i + 1]) {
            return i;
        }
    }

    return -1;
}

int Interpreter::getParentRow(int row) const {
    if (row < 0 || row >= mParentRow.size()) return -1;
    return mParentRow[row];   // 始终返回 -1
}

QVector<int> Interpreter::getChildRows(int parentRow) const {
    QVector<int> children;
    // 平级指令无父子行关系，始终返回空
    return children;
}