#pragma once
#include<qtemporaryfile.h>
#include <QWidget>
#include <QListWidget>
#include <QKeyEvent>
#include "ui_DSLEditor.h"
#include "../Interpreter.h"
#include "../ExceptionCollector.h"
#include "../SyntaxTree.h"
#include"Suggestion.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DSLEditorClass; };
QT_END_NAMESPACE

class DSLEditor : public QWidget {
    Q_OBJECT
public:
    explicit DSLEditor(QWidget* parent = nullptr);
    ~DSLEditor();

private slots:
    void processTextUpdate();
    bool modifyed = false;
private:
    Ui::DSLEditorClass* ui;
    QTemporaryFile m_tempFile;
    awf::Suggestion sug;
    QListWidget* m_completionPopup;
    QString loadPath;
    QString m_lastProcessedText;

    void applyNodeFormat(awf::TreeNode* node, QTextDocument* doc);
    QTextCharFormat formatForType(awf::TreeNode::Type t);

    QStringList getCompletionSuggestions(awf::TreeNode* node);
    void showCompletionPopup(const QStringList& suggestions);
    void hideCompletionPopup();
    void insertCompletion(const QString& text);

    bool eventFilter(QObject* obj, QEvent* event) override;   
signals:
    void beModifyed(DSLEditor* _this);
    void saved(DSLEditor* _this);
public:
   
    QString getLoadPath() {
        return loadPath;
    }
    bool loadFromFile(const QString& filePath);

    bool saveIntoFile(const QString& filePath);
    bool saveBack() {
        modifyed = false;
        emit saved(this);
        if (!loadPath.isEmpty()) {
            return saveIntoFile(loadPath);
        }return false;
    }
};