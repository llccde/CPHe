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
{
    ui->setupUi(this);

    // 创建补全弹出框（无焦点、无边框）
    m_completionPopup = new QListWidget(nullptr);
    // 改为 ToolTip，不抢键盘焦点
    m_completionPopup->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    m_completionPopup->setFocusPolicy(Qt::NoFocus);
    m_completionPopup->setMouseTracking(true);
    m_completionPopup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_completionPopup->setSelectionMode(QAbstractItemView::SingleSelection);
    // 可选，避免弹出时激活窗口
    m_completionPopup->setAttribute(Qt::WA_ShowWithoutActivating, true);

    // 安装事件过滤器，拦截编辑器按键
    ui->textEdit->installEventFilter(this);

    // 文本改变时直接进行统一的解析、高亮与补全
    connect(ui->textEdit, &QTextEdit::textChanged, this, &DSLEditor::processTextUpdate);
}

DSLEditor::~DSLEditor()
{
    delete ui;
}

void DSLEditor::processTextUpdate()
{
    // 1. 检查文本是否真正变化（避免重复解析）
    QString currentText = ui->textEdit->toPlainText();
    if (currentText == m_lastProcessedText)
        return;
    m_lastProcessedText = currentText;

    // 2. 将内容写入临时文件
    QTemporaryFile tempFile(QDir::tempPath() + "/dsleditor_XXXXXX.txt");
    tempFile.setAutoRemove(true);
    if (!tempFile.open())
        return;

    QTextStream out(&tempFile);
    out << currentText;
    tempFile.close();

    // 3. 创建解释器并解析（只创建一次）
    ExceptionCollector ec;
    Interpreter interpreter(ec);
    interpreter.loadFile(tempFile.fileName());
    awf::TreeNode* root = interpreter.rootNode();
    if (ec.hasErr()) ec.printAll();   // 仅用于调试，可注释

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

// ---------- 补全相关（未作实质性修改） ----------
QStringList DSLEditor::getCompletionSuggestions(awf::TreeNode* node)
{
    QStringList suggestions;
    if (!node) return suggestions;

    switch (node->type) {
    case awf::TreeNode::Root:
        suggestions << "@fill" << "@msg" << "@if" << "@for";
        break;
    case awf::TreeNode::CommandOperator:
        suggestions << "@fill" << "@msg" << "@if" << "@for";
        break;
    case awf::TreeNode::ArgList:
    case awf::TreeNode::ArgKey:
        suggestions << "color=" << "size=" << "text=";
        break;
    case awf::TreeNode::ArgVal:
        suggestions << "red" << "blue" << "12" << "\"hello\"";
        break;
    case awf::TreeNode::SingleArg:
    case awf::TreeNode::NaturalText:
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
    if (suggestions.isEmpty()) return;

    QStringList existingItems;
    for (int i = 0; i < m_completionPopup->count(); ++i)
        existingItems << m_completionPopup->item(i)->text();
    if (existingItems == suggestions && m_completionPopup->isVisible())
        return;

    m_completionPopup->clear();
    m_completionPopup->addItems(suggestions);
    m_completionPopup->setCurrentRow(0);

    QRect cursorRect = ui->textEdit->cursorRect();
    QPoint globalPos = ui->textEdit->mapToGlobal(cursorRect.bottomRight());
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        QRect screenGeom = screen->availableGeometry();
        int popupHeight = m_completionPopup->sizeHint().height();
        if (globalPos.y() + popupHeight > screenGeom.bottom())
            globalPos.setY(globalPos.y() - cursorRect.height() - popupHeight);
    }
    m_completionPopup->move(globalPos);
    m_completionPopup->show();
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