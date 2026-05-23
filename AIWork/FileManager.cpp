#include "FileManager.h"
#include <QFile>
#include <QTextStream>

namespace awf {

    // ----- 构造：从文件读取 -----
    LineBaseFileManager::LineBaseFileManager(const QString& path)
        : m_filePath(path)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);
            while (!in.atEnd())
                m_originalLines.append(in.readLine());
            file.close();
        }
        initRecords();
    }

    // ----- 构造：从字符串列表 -----
    LineBaseFileManager::LineBaseFileManager(const QStringList& content)
        : m_originalLines(content)
    {
        initRecords();
    }

    // ----- 初始化插入桶 -----
    void LineBaseFileManager::initRecords()
    {
        m_records.resize(m_originalLines.size() + 1);
    }

    // ----- 写回原文件 -----
    bool LineBaseFileManager::writeBack()
    {
        if (m_filePath.isEmpty())
            return false;
        return writeTo(m_filePath);
    }

    // ----- 写入指定文件 -----
    bool LineBaseFileManager::writeTo(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return false;

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        const QStringList lines = getContent();
        for (int i = 0; i < lines.size(); ++i) {
            out << lines[i];
            if (i != lines.size() - 1)
                out << "\n";               // 避免最后产生一个空行
        }
        file.close();
        return true;
    }

    // ----- 在原始行 line 之后插入 -----
    void LineBaseFileManager::insertAfterLineOfOrigin(int line, const QString& content)
    {
        if (line < 0 || line >= m_originalLines.size())
            return;                         // 越界忽略，也可改为断言
        m_records[line + 1].append({ content });
    }

    // ----- 在原始行 line 之前插入 -----
    void LineBaseFileManager::insertBeforeLineOfOrigin(int line, const QString& content)
    {
        if (line < 0 || line >= m_originalLines.size())
            return;
        m_records[line].append({ content });
    }

    // ----- 获取完整内容 -----
    QStringList LineBaseFileManager::getContent() const
    {
        QStringList result;
        const int origCount = m_originalLines.size();
        for (int i = 0; i < origCount; ++i) {
            // 插入在第 i 行之前的行
            for (const auto& rec : m_records[i])
                result.append(rec.content);
            // 原始行 i
            result.append(m_originalLines[i]);
        }
        // 文件末尾插入的行（桶索引 = origCount）
        for (const auto& rec : m_records[origCount])
            result.append(rec.content);

        return result;
    }

    // ----- 原始行数 -----
    int LineBaseFileManager::originalLineCount() const
    {
        return m_originalLines.size();
    }

} // namespace awf