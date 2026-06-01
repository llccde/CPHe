#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QString>
#include <QDebug>
#include"M_Command.h"
namespace awf {


    class CommentTool {
    public:
    QVector<LineInfo> analyzeFile(const QString & filePath,
        const QString & singleLine,
        const QString & blockStart,
        const QString & blockEnd)
         {
             QVector<LineInfo> result;

             // 1. 读取文件所有行
             QFile file(filePath);
             if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                 qWarning() << "Cannot open file:" << filePath;
                 return result;
             }
             QTextStream in(&file);
             while (!in.atEnd()) {
                 LineInfo info;
                 info.rawLine = in.readLine() + "\n";   // 保留换行符
                 result.append(info);
             }
             file.close();

             if (result.isEmpty())
                 return result;

             // 2. 逐行解析，状态机处理块注释
             bool inBlock = false;               // 是否处于块注释内部
             int blockStartLine = -1;            // 块注释起始行索引（0-based）
             QStringList blockBuffer;            // 暂存块注释内的行文本（不含注释符号）

             for (int i = 0; i < result.size(); ++i) {
                 const QString& raw = result[i].rawLine;
                 // 去掉行首的空白（用于判断是否以注释符开头）
                 QString trimmed = raw.trimmed();

                 if (!inBlock) {
                     // 不在块注释内 → 检查单行注释或块注释开始
                     if (trimmed.startsWith(singleLine)) {
                         // 单行注释：整行都是注释指令
                         QString content = trimmed.mid(singleLine.length());
                         // 去除结尾的换行符并清理首尾空白
                         content.remove('\n');
                         result[i].commentText = content.trimmed();
                         result[i].isCommentOnly = true;
                     }
                     else if (trimmed.startsWith(blockStart)) {
                         // 块注释开始：从当前行进入多行模式
                         inBlock = true;
                         blockStartLine = i;
                         blockBuffer.clear();

                         // 处理当前行内可能存在的块结束符（单行块注释情况）
                         int startPos = raw.indexOf(blockStart);
                         int endPos = raw.indexOf(blockEnd, startPos + blockStart.length());
                         if (endPos != -1) {
                             // 起始和结束在同一行 → 不跨行
                             QString content = raw.mid(startPos + blockStart.length(), endPos - startPos - blockStart.length());
                             content.remove('\n');
                             result[i].commentText = content.trimmed();
                             result[i].hasMutiLineCommentBegin = true;
                             result[i].hasMutiLineCommentEnd = true;
                             // 检查结束符后面是否有代码（非空白）
                             QString afterEnd = raw.mid(endPos + blockEnd.length());
                             if (afterEnd.trimmed().isEmpty())
                                 result[i].isCommentOnly = true;
                             else
                                 result[i].isCommentOnly = false;   // 行尾有代码，不是纯注释行
                             inBlock = false;   // 已闭合，无需继续
                         }
                         else {
                             // 未在本行闭合 → 提取起始符后的部分作为块注释内容暂存
                             QString content = raw.mid(startPos + blockStart.length());
                             content.remove('\n');
                             blockBuffer.append(content);
                             result[i].hasMutiLineCommentBegin = true;
                             result[i].isCommentOnly = true;  // 起始行无代码（已去除行首空白）
                         }
                     }
                     // 否则为普通代码行，保持默认值
                 }
                 else {
                     // 已在块注释内部 → 查找闭合符
                     int endPos = raw.indexOf(blockEnd);
                     if (endPos != -1) {
                         // 找到闭合符：将当前行闭合符之前的部分加入缓冲，然后统一处理
                         QString content = raw.left(endPos);
                         content.remove('\n');
                         blockBuffer.append(content);
                         // 将缓冲中的所有行注释文本分配到对应行
                         int lineIdx = blockStartLine;
                         for (const QString& part : blockBuffer) {
                             if (lineIdx >= result.size()) break;
                             if (!result[lineIdx].commentText.isEmpty())
                                 result[lineIdx].commentText += " ";
                             result[lineIdx].commentText += part.trimmed();
                             result[lineIdx].hasMutiLineCommentBegin = (lineIdx == blockStartLine);
                             result[lineIdx].hasMutiLineCommentEnd = (lineIdx == i);
                             // 块注释内部的行，若没有代码（实际上整行都是注释）则标记为纯注释行
                             // 但需检查闭合符后是否有代码
                             if (lineIdx == i) {
                                 // 闭合行：检查闭合符后面是否有非空白字符
                                 QString afterEnd = raw.mid(endPos + blockEnd.length());
                                 result[lineIdx].isCommentOnly = afterEnd.trimmed().isEmpty();
                             }
                             else {
                                 result[lineIdx].isCommentOnly = true;
                             }
                             ++lineIdx;
                         }
                         // 重置状态
                         inBlock = false;
                         blockBuffer.clear();
                         blockStartLine = -1;
                     }
                     else {
                         // 未闭合：整行都属于块注释内容
                         QString content = raw;
                         content.remove('\n');
                         blockBuffer.append(content);
                         result[i].isCommentOnly = true;   // 中间行无代码
                     }
                 }
             }

             // 若文件结束时仍处于未闭合的块注释中，可根据需要处理（本例忽略）
             if (inBlock) {
                 qWarning() << "Unclosed block comment starting at line" << blockStartLine + 1;
             }

             return result;
         }
    };
}
