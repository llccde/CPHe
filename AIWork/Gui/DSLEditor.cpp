#include "DSLEditor.h"
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTemporaryFile>
#include <QTextStream>
#include <QDir>
#include <QScreen>
#include <QGuiApplication>

using namespace awf;

DSLEditor::DSLEditor(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DSLEditorClass)
    , sug(QString("E:\\cpp\\qt\\CPHe\\AIWork"))
{
    ui->setupUi(this);

    // ---------- 临时文件初始化（不变） ----------
    m_tempFile.setFileTemplate(QDir::tempPath() + "/dsleditor_XXXXXX.txt");
    m_tempFile.setAutoRemove(false);
    if (m_tempFile.open()) {
        m_tempFile.close();
    }

    // ---------- 补全弹出框（改为普通子 Widget） ----------
    m_completionPopup = new QListWidget(this);
    // 设置为普通 Widget，不再使用 Qt::ToolTip
    m_completionPopup->setWindowFlags(Qt::Widget);          // 默认就是 Widget，可省略
    m_completionPopup->setFocusPolicy(Qt::NoFocus);         // 不抢键盘焦点
    m_completionPopup->setMouseTracking(true);
    m_completionPopup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_completionPopup->setSelectionMode(QAbstractItemView::SingleSelection);
    // 保证它在父控件内部绘制时位于最上层（配合 show 时 raise()）
    m_completionPopup->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    // 初始隐藏
    m_completionPopup->hide();

    // 安装事件过滤器，拦截编辑器按键
    ui->textEdit->installEventFilter(this);

    // 文本改变时直接进行统一的解析、高亮与补全
    connect(ui->textEdit, &QTextEdit::textChanged,
        this, &DSLEditor::processTextUpdate);
}

DSLEditor::~DSLEditor()
{
    if (m_tempFile.exists())
        m_tempFile.remove();
    delete ui;
}

void DSLEditor::processTextUpdate()

{
    // 1. 检查文本是否真正变化（避免重复解析）
    QString currentText = ui->textEdit->toPlainText();
    if (currentText == m_lastProcessedText)
        return;
    m_lastProcessedText = currentText;
    if (!modifyed) {
        modifyed = true;
        emit beModifyed(this);
    }
    

    QFile writer(m_tempFile.fileName());
    if (!writer.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;                             // 写入失败则放弃本次解析

    QTextStream out(&writer);
    out << currentText;
    writer.close(); 

    // 4. 加载同一个临时文件进行解析
    ExceptionCollector ec;
    Interpreter interpreter(ec);
    interpreter.loadFile(m_tempFile.fileName()); // 传入文件名即可
    awf::TreeNode* root = interpreter.rootNode();


    if (ec.hasErr()) ec.printAll();

    // 4. 语法高亮（清除旧格式，应用新格式）
    QTextDocument* doc = ui->textEdit->document();
    {
        QTextCursor cursor(doc);
        cursor.select(QTextCursor::Document);
        cursor.setCharFormat(QTextCharFormat());
    }
    if (root)
        applyNodeFormat(root, doc);

    // 5. 代码补全
    if (!root) {
        hideCompletionPopup();
        return;
    }

    // 获取光标所在位置的 AST 节点
    QTextCursor cursor = ui->textEdit->textCursor();
    int pos = cursor.position();
    QTextBlock block = cursor.block();
    int line = block.blockNumber();
    int col = pos - block.position();
    awf::TreeNode* deepest = root->deepestNodeAt(line, col);
    if (!deepest && col - 1 >= 0) deepest = root->deepestNodeAt(line, col - 1);
    QStringList suggestions = getCompletionSuggestions(deepest);
    if (suggestions.isEmpty())
        hideCompletionPopup();
    else
        showCompletionPopup(suggestions);
}

// ---------- 语法高亮（与原来相同，保留） ----------
void DSLEditor::applyNodeFormat(awf::TreeNode* node, QTextDocument* doc)
{
    if (!node) return;

    if (node->type == awf::TreeNode::Root) {
        for (awf::TreeNode* child : node->children)
            applyNodeFormat(child, doc);
        return;
    }

    if (node->lineBegin < 0 || node->colBegin < 0) {
        for (awf::TreeNode* child : node->children)
            applyNodeFormat(child, doc);
        return;
    }

    QTextCharFormat fmt = formatForType(node->type);

    if (node->lineBegin == node->lineEnd) {
        QTextBlock block = doc->findBlockByNumber(node->lineBegin);
        if (!block.isValid()) return;
        int start = block.position() + node->colBegin;
        int end = block.position() + qMin(node->colEnd, block.length() - 1);
        QTextCursor cursor(doc);
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(fmt);
    }
    else {
        for (int line = node->lineBegin; line <= node->lineEnd; ++line) {
            QTextBlock block = doc->findBlockByNumber(line);
            if (!block.isValid()) continue;
            int startCol = (line == node->lineBegin) ? node->colBegin : 0;
            int endCol;
            if (line == node->lineEnd) {
                endCol = (node->colEnd >= 0) ? node->colEnd : (block.length() - 1);
            }
            else {
                endCol = block.length() - 1;
            }
            int start = block.position() + startCol;
            int end = block.position() + qMin(endCol, block.length() - 1);
            QTextCursor cursor(doc);
            cursor.setPosition(start);
            cursor.setPosition(end, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(fmt);
        }
    }

    for (awf::TreeNode* child : node->children)
        applyNodeFormat(child, doc);
}

QTextCharFormat DSLEditor::formatForType(awf::TreeNode::Type t)
{
    QTextCharFormat fmt;
    switch (t) {
    case awf::TreeNode::CommandOperator:
        fmt.setForeground(QColor(0, 128, 255));
        fmt.setFontWeight(QFont::Bold);
        break;
    case awf::TreeNode::ArgKey:
        fmt.setForeground(QColor(160, 0, 160));
        break;
    case awf::TreeNode::ArgVal:
        fmt.setForeground(QColor(0, 128, 0));
        break;
    case awf::TreeNode::SingleArg:
        fmt.setForeground(QColor(120, 120, 120));
        fmt.setFontItalic(true);
        break;
    case awf::TreeNode::NaturalText:
        fmt.setForeground(QColor(100, 100, 100));
        break;
    case awf::TreeNode::Error:
        fmt.setUnderlineColor(Qt::red);
        fmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        break;
    default:
        break;
    }
    return fmt;
}

QStringList DSLEditor::getCompletionSuggestions(awf::TreeNode* node)
{
    QStringList suggestions;
    if (!node) return suggestions;

    switch (node->type) {
    case awf::TreeNode::Root:
        // 根节点暂不提供补全
        break;

    case awf::TreeNode::Command:
        // 正在输入指令名，返回所有可用操作符
        return sug.getOperator("");

    case awf::TreeNode::CommandOperator:
        // 正在输入操作符，继续补全操作符本身
        return sug.getOperator(node->text.trimmed());

    case awf::TreeNode::ArgList:
    {
        // 获取所属 Command，并取出其中的操作符节点
        TreeNode* cmd = node->getParentOfType(TreeNode::Command);
        if (cmd->isValid) {
            TreeNode* op = cmd->getChildOfType(TreeNode::CommandOperator);
            if (op->isValid) {
                return sug.getArgs(op->text.trimmed(), "");
            }
        }
        break;
    }

    case awf::TreeNode::ArgKey:
    {
        // 当前正在输入参数名，需要操作符信息和已输入的部分键名
        TreeNode* cmd = node->getParentOfType(TreeNode::Command);
        if (cmd->isValid) {
            TreeNode* op = cmd->getChildOfType(TreeNode::CommandOperator);
            if (op->isValid) {
                return sug.getArgs(op->text.trimmed(), node->text.trimmed());
            }
        }
        break;
    }

    case awf::TreeNode::ArgVal:
    {
        TreeNode* cmd = node->getParentOfType(TreeNode::Command);
        if (cmd->isValid) {
            TreeNode* op = cmd->getChildOfType(TreeNode::CommandOperator);
            TreeNode* argItem = node->getParentOfType(TreeNode::ArgItem);
            if (argItem->isValid) {
                TreeNode* keyNode = argItem->getChildOfType(TreeNode::ArgKey);
                if (keyNode->isValid && op->isValid) {
                    return sug.getArgValue(op->text, keyNode->text.trimmed(), node->text.trimmed());
                }
            }
        }
        break;
    }

    case awf::TreeNode::SingleArg:
        suggestions << "example" << "description";
        break;

    case awf::TreeNode::Error:
        suggestions << "@fill" << "@msg";
        break;

    default:
        break;
    }
    return suggestions;
}

void DSLEditor::showCompletionPopup(const QStringList& suggestions)
{
    if (suggestions.isEmpty())
        return;

    // 避免重复刷新相同内容
    QStringList existingItems;
    for (int i = 0; i < m_completionPopup->count(); ++i)
        existingItems << m_completionPopup->item(i)->text();
    if (existingItems == suggestions && m_completionPopup->isVisible())
        return;

    m_completionPopup->clear();
    m_completionPopup->addItems(suggestions);
    m_completionPopup->setCurrentRow(0);

    // 计算位置与大小（相对 DSLEditor 的本地坐标）
    QRect cursorRect = ui->textEdit->cursorRect();
    // 光标矩形右下角在 QTextEdit 内部坐标
    QPoint cursorBottomRight = cursorRect.bottomRight();
    // 转换到 DSLEditor (this) 的坐标系
    QPoint localPos = ui->textEdit->mapTo(this, cursorBottomRight);

    // 设定补全框的推荐宽度（可根据需要调整）
    m_completionPopup->setFixedWidth(220);
    // 根据内容调整高度（但不超过可见区域）
    m_completionPopup->adjustSize();

    int popupW = m_completionPopup->width();
    int popupH = m_completionPopup->sizeHint().height();

    // 边界检查：确保补全框不超出 DSLEditor 的可视范围
    QRect editorRect = rect();   // 本地坐标

    int x = localPos.x();
    int y = localPos.y();

    // 水平边界：若右侧超出则向左偏移
    if (x + popupW > editorRect.right())
        x = editorRect.right() - popupW;
    if (x < editorRect.left())
        x = editorRect.left();

    // 垂直边界：默认在光标下方显示，若下方空间不够则显示在光标上方
    if (y + popupH > editorRect.bottom()) {
        // 改为光标上方
        y = localPos.y() - cursorRect.height() - popupH;
        if (y < editorRect.top())
            y = editorRect.top();
    }

    m_completionPopup->move(x, y);
    m_completionPopup->show();
    m_completionPopup->raise();   // 确保在所有兄弟控件之上
}

void DSLEditor::hideCompletionPopup()
{
    if (m_completionPopup->isVisible())
        m_completionPopup->hide();
}

void DSLEditor::insertCompletion(const QString& text)
{
    if (text.isEmpty()) return;
    QTextCursor cursor = ui->textEdit->textCursor();
    int pos = cursor.selectionStart();   // 插入点（有选择则为选择起点，否则为光标位置）
    int end = cursor.selectionEnd();     // 选择结束点（无选择时等于 pos）
    // 获取插入点前至多 text.length() 个字符作为“已有后缀”
    int prefixLen = qMin(pos, text.length());
    QString toInsert = text;             // 默认插入原文本
    if (prefixLen > 0) {
        QTextCursor tmp(ui->textEdit->document());
        tmp.setPosition(pos);
        tmp.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefixLen);
        const QString prefix = tmp.selectedText();
        // 寻找已有后缀与补全文本前缀的最长重叠长度
        int overlap = 0;
        for (int i = prefixLen; i > 0; --i) {
            if (prefix.endsWith(text.left(i))) {
                overlap = i;
                break;
            }
        }
        toInsert = text.mid(overlap);   // 截去重叠部分
    }
    // 执行插入（若原先有选区则自动替换选区内容）
    cursor.setPosition(end);
    cursor.setPosition(pos, QTextCursor::KeepAnchor);
    cursor.insertText(toInsert);
    ui->textEdit->setTextCursor(cursor);
}

bool DSLEditor::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (m_completionPopup->isVisible()) {
            switch (keyEvent->key()) {
            case Qt::Key_Up:
                if (m_completionPopup->currentRow() > 0)
                    m_completionPopup->setCurrentRow(m_completionPopup->currentRow() - 1);
                return true;
            case Qt::Key_Down:
                if (m_completionPopup->currentRow() < m_completionPopup->count() - 1)
                    m_completionPopup->setCurrentRow(m_completionPopup->currentRow() + 1);
                return true;
            //case Qt::Key_Return:
            case Qt::Key_Tab: {
                QListWidgetItem* item = m_completionPopup->currentItem();
                if (item)
                    insertCompletion(item->text());
                hideCompletionPopup();
                return true;
            }
            case Qt::Key_Escape:
                hideCompletionPopup();
                return true;
            default:
                if (!keyEvent->text().isEmpty() ||
                    keyEvent->key() == Qt::Key_Backspace ||
                    keyEvent->key() == Qt::Key_Delete) {
                    hideCompletionPopup();
                }
                break;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
bool DSLEditor::loadFromFile(const QString& filePath)
{

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    // 设置文本会触发 textChanged 信号，进而自动进行解析、高亮与补全
    ui->textEdit->setPlainText(content);
    loadPath = filePath;
    return true;
}
bool DSLEditor::saveIntoFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << ui->textEdit->toPlainText();
    file.close();
    return true;
}