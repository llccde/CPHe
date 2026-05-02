// SubStr.h
#ifndef SUBSTR_H
#define SUBSTR_H

#include <QString>
#include <QChar>
#include <algorithm>

class SubStr {
public:
    SubStr() : src(nullptr), begin(0), end(0) {}
    SubStr(const QString& source, int b, int e)
        : src(&source), begin(b), end(e) {
    }

    const QString& source() const { return *src; }
    int size() const { return end - begin; }
    bool isEmpty() const { return size() == 0; }

    QChar at(int i) const { return src->at(begin + i); }
    QChar operator[](int i) const { return at(i); }

    // 从当前 SubStr 内部相对位置切割子串
    SubStr mid(int pos, int length = -1) const {
        int absStart = begin + pos;
        int absEnd = (length < 0) ? end : std::min(absStart + length, end);
        return SubStr(*src, absStart, absEnd);
    }

    // 去除左右空白
    SubStr trimmed() const {
        int b = begin;
        int e = end;
        while (b < e && src->at(b).isSpace()) ++b;
        while (e > b && src->at(e - 1).isSpace()) --e;
        return SubStr(*src, b, e);
    }

    int indexOf(QChar c, int from = 0) const {
        for (int i = from; i < size(); ++i) {
            if (at(i) == c) return i;
        }
        return -1;
    }

    bool startsWith(QChar c) const {
        return !isEmpty() && at(0) == c;
    }

    // 转换为独立的 QString（仅在最终组装结果时使用）
    QString toString() const {
        if (!src)return"";
        return src->mid(begin, end - begin);
    }

    // 获取绝对位置
    int absoluteBegin() const { return begin; }
    int absoluteEnd() const { return end; }

private:
    const QString* src;
    int begin;
    int end;
};

#endif // SUBSTR_H