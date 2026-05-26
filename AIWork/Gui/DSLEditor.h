#pragma once

#include <QWidget>
#include <QListWidget>
#include <QKeyEvent>
#include "ui_DSLEditor.h"
#include "../Interpreter.h"
#include "../ExceptionCollector.h"
#include "../SyntaxTree.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DSLEditorClass; };
QT_END_NAMESPACE

class DSLEditor : public QWidget {
    Q_OBJECT
public:
    explicit DSLEditor(QWidget* parent = nullptr);
    ~DSLEditor();

private slots:
    void processTextUpdate();   // 统一的文本更新处理

private:
    Ui::DSLEditorClass* ui;

    // 补全弹出框
    QListWidget* m_completionPopup;

    // 缓存的文本，用于避免重复解析
    QString m_lastProcessedText;

    // 语法高亮相关
    void applyNodeFormat(awf::TreeNode* node, QTextDocument* doc);
    QTextCharFormat formatForType(awf::TreeNode::Type t);

    // 补全相关
    QStringList getCompletionSuggestions(awf::TreeNode* node);
    void showCompletionPopup(const QStringList& suggestions);
    void hideCompletionPopup();
    void insertCompletion(const QString& text);

    bool eventFilter(QObject* obj, QEvent* event) override;
};