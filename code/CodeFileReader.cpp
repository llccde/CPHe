#include"CodeFileReader.h"
#include"qfile.h"
#include"qtextstream.h"
QString readContentFromPosition(const CodePosition& pos) {
    // 打开文件
    QFile file(pos.file);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString(); // 打开失败，返回空
    }

    QTextStream stream(&file);
    // 可选：设置编码，例如 stream.setCodec("UTF-8");

    QString result;
    unsigned currentRow = 1; // 行号从 1 开始

    while (!stream.atEnd()) {
        QString line = stream.readLine(); // 读取一行，不含换行符
        bool lineHandled = false;

        // 判断当前行是否在区间内
        if (currentRow >= pos.rowBegin && currentRow <= pos.rowEnd) {
            // 转换为 0-based 列索引
            int colBegin = static_cast<int>(pos.columBegin) - 1;
            int colEnd = static_cast<int>(pos.columnEnd) - 1;

            // 起始行与结束行是否相同
            if (pos.rowBegin == pos.rowEnd) {
                // 同一行：截取 [colBegin, colEnd]
                if (colBegin < line.size()) {
                    int len = qMin(colEnd - colBegin + 1, line.size() - colBegin);
                    if (len > 0) {
                        result += line.mid(colBegin, len);
                    }
                }
                lineHandled = true; // 处理完毕，无需再处理其他情况
            }
            else {
                // 跨多行
                if (currentRow == pos.rowBegin) {
                    // 起始行：从 colBegin 到行末
                    if (colBegin < line.size()) {
                        result += line.mid(colBegin);
                    }
                }
                else if (currentRow == pos.rowEnd) {
                    // 结束行：从行首到 colEnd
                    if (colEnd >= 0) {
                        int len = qMin(colEnd + 1, line.size());
                        if (len > 0) {
                            result += line.left(len);
                        }
                    }
                }
                else {
                    // 中间行：整行
                    result += line;
                }
            }
        }

        // 添加换行符（除了最后一行）
        // 注意：如果当前行是区间的一部分，并且不是区间的最后一行，则添加换行符
        if (currentRow >= pos.rowBegin && currentRow < pos.rowEnd && !lineHandled) {
            result += '\n';
        }

        ++currentRow;

        // 如果已超过区间结束行，可以提前终止（但需要确保已添加换行符）
        if (currentRow > pos.rowEnd) {
            break;
        }
    }

    file.close();
    return result;
}