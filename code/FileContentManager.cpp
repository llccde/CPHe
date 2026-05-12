#include "FileContentManager.h"
#include <QFileInfo>
#include<qdir.h>
void FileContentManager::accept(const FileRange& range) {
    m_operations.append({ true, range });
}

void FileContentManager::removeAccept(const FileRange& range) {
    m_operations.append({ false, range });
}

QVector<QString> FileContentManager::applyOperationsForFile(const QString& filePath,
    const QVector<QString>& originContent) const {
    const int numRows = originContent.size();
    if (numRows == 0)
        return {};

    QVector<int> lineLengths(numRows);
    for (int i = 0; i < numRows; ++i)
        lineLengths[i] = originContent[i].length();

    QVector<QVector<bool>> accepted(numRows);
    for (int i = 0; i < numRows; ++i)
        accepted[i].resize(lineLengths[i], false);

    QString targetPath = QFileInfo(filePath).canonicalFilePath();
    if (targetPath.isEmpty()) {
        targetPath = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    }

    for (const auto& op : m_operations) {
        QString opPath = QFileInfo(op.range.file).canonicalFilePath();
        if (opPath.isEmpty()) {
            opPath = QDir::cleanPath(QFileInfo(op.range.file).absoluteFilePath());
        }
        if (opPath != targetPath)
            continue;

        const FileRange& r = op.range;
        const bool setVal = op.isAccept;

        int startRow = qMax(0, r.row);
        int endRow = qMin(numRows - 1, r.rowEnd);
        if (startRow > endRow)
            continue;

        for (int i = startRow; i <= endRow; ++i) {
            int len = lineLengths[i];
            if (len == 0)
                continue;

            int startCol = (i == r.row) ? qMax(0, r.column) : 0;
            int endCol = (i == r.rowEnd) ? qMin(len - 1, r.columnEnd) : (len - 1);
            if (startCol > endCol)
                continue;

            for (int j = startCol; j <= endCol; ++j)
                accepted[i][j] = setVal;
        }
    }

    QVector<QString> result(numRows);
    for (int i = 0; i < numRows; ++i) {
        const QString& src = originContent[i];
        QString line;
        for (int j = 0; j < lineLengths[i]; ++j) {
            if (accepted[i][j])
                line += src[j];
        }
        result[i] = line;
    }
    return result;
}
QVector<QString> FileContentManager::readFile(const QString& filePath, const QVector<QString>& originContent) {
    return applyOperationsForFile(filePath, originContent);
}

QMap<QString, QVector<QString>> FileContentManager::readFiles(const QMap<QString, QVector<QString>>& originContents) {
    QMap<QString, QVector<QString>> result;
    for (auto it = originContents.begin(); it != originContents.end(); ++it) {
        result.insert(it.key(), applyOperationsForFile(it.key(), it.value()));
    }
    return result;
}
