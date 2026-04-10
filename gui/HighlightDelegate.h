#ifndef HIGHLIGHTDELEGATE_H
#define HIGHLIGHTDELEGATE_H

#include <QStyledItemDelegate>
#include <QSet>
#include <QString>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPainter>
#include <QPalette>
#include <QFont>
//或许不用拆分一个cpp文件
class HighlightDelegateOfQString : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit HighlightDelegateOfQString(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    // 添加需要高亮的字符串（精确匹配）
    void addHighlightString(const QString& text)
    {
        m_highlightStrings.insert(text);
    }

    // 移除指定字符串的高亮
    void removeHighlightString(const QString& text)
    {
        m_highlightStrings.remove(text);
    }

    // 清除所有高亮字符串
    void clearAllHighlightStrings()
    {
        m_highlightStrings.clear();
    }

    // 检查指定字符串是否会被高亮（可选，供外部使用）
    bool isHighlightString(const QString& text) const
    {
        return m_highlightStrings.contains(text);
    }

    // 重写 paint 方法：若当前项显示的文本与任一高亮字符串相等，则应用深绿色加粗样式
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        // 获取当前项显示的文本（默认 DisplayRole）
        QString text = index.data(Qt::DisplayRole).toString();

        if (m_highlightStrings.contains(text)) {
            QStyleOptionViewItem opt = option;
            // 设置加粗字体
            QFont font = opt.font;
            font.setBold(true);
            opt.font = font;
            // 设置深绿色文字

            opt.palette.setColor(QPalette::Text, QColor(0, 100, 0));
            QStyledItemDelegate::paint(painter, opt, index);
        }
        else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

private:
    QSet<QString> m_highlightStrings;   // 存储需要高亮的字符串
};

#endif // HIGHLIGHTDELEGATE_H