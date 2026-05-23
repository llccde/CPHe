#include"AIWorkFlow.h"
#include "AIWorkFlow.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include"FileManager.h"
#include"BaseTool.h"
using namespace awf;
// AIWorkFlow.cpp
awf::AIWorkFlow::AIWorkFlow(const QString& working)
    : workingFolder(working), fileProcesser(ec), aic(ec), tool(ec)
{
}

QString awf::AIWorkFlow::getAbsPath(const QString& path) {
    return QDir(workingFolder).absoluteFilePath(path);
}

QString awf::AIWorkFlow::readKey(QString key) {
    QFile f(getAbsPath(key));
    if (f.exists() && !f.atEnd()) {
        QString s(f.readLine());
        if (s.endsWith("\n")) s.chop(1);
        return s;
    }
    riseError(getAbsPath(key) + " not found");
    return "";
}

QString awf::AIWorkFlow::getGenId() {
    return QString::number(genId++);
}

int awf::AIWorkFlow::getCurrentIndex() {
    return index;
}

M_Command awf::AIWorkFlow::getCurrentCommand() {
    return fileProcesser.getCommandOf(index);
}

QString awf::AIWorkFlow::currentSource() {
    return fileProcesser.getSource(index);
}

M_Command awf::AIWorkFlow::peekNext() {
    return fileProcesser.getCommandOf(index + 1);
}

void awf::AIWorkFlow::next(bool write) {
    index++;
    if (write) writeFile(fileProcesser.getSource(index));
    auto c = getCurrentCommand();
    if (c.hasMutiLineCommentEnd) inCommentBlock = false;
    if (c.hasMutiLineCommentBegin) inCommentBlock = true;
}

bool awf::AIWorkFlow::hasNext() {
    return index + 1 < fileProcesser.rowCount();
}

void awf::AIWorkFlow::writeFile(const QString& data, int index) {
    auto clean = tool.clearSingleLineBreak(data);
    if (index != -1 && index < fileProcesser.rowCount()) {
        auto data_f = tool.SameTab(clean, fileProcesser.getSource(index));
        fileBuffer.append(data_f + "\n");
        return;
    }
    fileBuffer.append(clean + "\n");
}

void awf::AIWorkFlow::writeComment(const QString& data, int index) {
    if (!inCommentBlock) {
        writeFile("//" + data, index);
    }
    else {
        writeFile(data, index);
    }
}

void awf::AIWorkFlow::writeComment(const QVector<QString> data, int index) {
    if (!inCommentBlock) {
        auto dataC = data;
        tool.clearLineBreak(dataC);
        if (index != -1 && index < fileProcesser.rowCount()) {
            tool.FormatTab(dataC, fileProcesser.getSource(index));
        }
        fileBuffer.append("/*" + dataC.join("\n") + "*/\n");
    }
    else {
        writeFile(data, index);
    }
}

void awf::AIWorkFlow::writeSource(const QString& data, int index) {
    writeSource({ data }, index);
}

void awf::AIWorkFlow::writeSource(const QVector<QString> data, int index) {
    if (inCommentBlock) {
        auto dataC = data;
        dataC.push_front("*/");
        dataC.push_back("/*");
        tool.clearLineBreak(dataC);
        if (index != -1 && index < fileProcesser.rowCount()) {
            tool.FormatTab(dataC, fileProcesser.getSource(index));
        }
        fileBuffer.append(dataC.join("\n") + '\n');
    }
    else {
        writeFile(data, index);
    }
}

void awf::AIWorkFlow::writeFile(const QVector<QString>& data, int index) {
    auto dataC = data;
    tool.clearLineBreak(dataC);
    if (index != -1 && index < fileProcesser.rowCount()) {
        tool.FormatTab(dataC, fileProcesser.getSource(index));
    }
    fileBuffer.append(dataC.join("\n") + "\n");
}

void awf::AIWorkFlow::riseWarn(const QString& wrn) {
    ec.riseWrn(wrn);
}

void awf::AIWorkFlow::riseError(const QString& err) {
    ec.riseErr(err);
}

bool awf::AIWorkFlow::hasError() {
    return ec.hasErr();
}

void awf::AIWorkFlow::writeCurrentSource() {
    writeFile(fileProcesser.getSource(index));
}

M_Command awf::AIWorkFlow::justNextCommand() {
    next();
    return getCurrentCommand();
}

M_Command awf::AIWorkFlow::skipNextCommand() {
    next(false);
    return getCurrentCommand();
}

void awf::AIWorkFlow::newFileCommand() {
    while (true) {
        if (!hasNext()) return;
        if (hasError()) return;
        switch (peekNext().type) {
        case MP::notCommand:
        case MP::normalComment:
            justNextCommand();
            break;
        case MP::fill:
            fillCommand();
            break;
        case MP::genBegin:
            handleGenLable();
            break;
        case MP::genEnd:
            riseError("未匹配的 @genEnd 标签");
            return;
        case MP::debugger:
            debugger();
            break;
        default:
            riseWarn("不支持 其他指令标记 在 文档根节点下");
            justNextCommand();
            break;
        }
    }
}

void awf::AIWorkFlow::debugger() {
    auto current = getCurrentCommand();
    next();
    auto next = peekNext();
    (void)current; (void)next; // 避免未使用变量警告
    return;
}

void awf::AIWorkFlow::handleGenLable() {
    next(false);
    auto _this = getCurrentCommand();
    bool findEnd = false;
    while (true) {
        if (!hasNext()) {
            riseError("文件结尾处未闭合的 @genBegin 指令标记");
            return;
        }
        auto next = peekNext();
        switch (next.type) {
        case MP::genBegin:
            if (next.getArg("id") == _this.getArg("id")) {
                riseError("非预期匹配的 @genBegin 指令标记");
                return;
            }
            skipNextCommand();
            break;
        case MP::genEnd:
            if (next.getArg("id") == _this.getArg("id")) {
                findEnd = true;
            }
            skipNextCommand();
            break;
        default:
            skipNextCommand();
            break;
        }
        if (findEnd) break;
    }
}

void awf::AIWorkFlow::fillCommand() {
    next();
    int row = getCurrentIndex();
    auto genID = getGenId();
    auto cur = getCurrentCommand();

    enum Named { Class, Func, NotNamed } namedState = NotNamed;
    QString nameSymbol;
    (void)nameSymbol; // 避免未使用变量警告

    QString modelName = "";
    QVector<ChatMessage> promot;
    ChatMessage userMessage = {user, ""};
    userMessage.message.append(cur.arg);
    promot.append({ system,"编写的代码务必使用```cpp和 ``` 包裹" });
    if (cur.isLongOperator) {
        bool findEnd = false;
        while (!findEnd) {
            if (!hasNext()) {
                riseWarn("文件结尾处未闭合的 长指令标记");
                break;
            }
            switch (peekNext().type) {
            case MP::ref: {
                auto data = handleRef();
                promot.append({system,"参考定义:\n"+data});
                break;
            }
            case MP::msg:
            case MP::normalComment: {
                auto text = justNextCommand();
                userMessage.message.append(text.arg);
                break;
            }
            case MP::nameFunc: {
                if (namedState == NotNamed) {
                    auto name = justNextCommand();
                    promot.append({system,QString("用户指定生成的函数必须名为\"%1\",不要有任何不同").arg(name.arg)});
                    nameSymbol = name.arg;
                    namedState = Func;
                }
                else {
                    riseWarn("只能指定一个代码符号的名称");
                    justNextCommand();
                }
                break;
            }
            case MP::moduleName: {
                modelName = justNextCommand().arg;
                break;
            }
            case MP::end:
                findEnd = true;
                justNextCommand();
                break;
            default:
                riseWarn("在@fill 长指令标记 区间内,除去@ref,@end,不支持任何其他指令");
                justNextCommand();
                break;
            }
        }
    }
    QVector<QString> args;
    args.append("op = fill");
    args.append("id = genID");
    if (!nameSymbol.isEmpty()) {
        args.append("symbolName = " + nameSymbol);
    }
    if (!modelName.isEmpty()) {
        args.append("modelName = " + modelName);
    }
    promot.append(userMessage);
    writeComment("@genBegin,"+args.join(","),row);
    // 注意：原代码中使用了未定义的 'user'，此处保持原样
    
    writeSource(extract("```cpp", "```", aic.getGen(promot, genFunc).split("\n")),row);
    writeComment("@genEnd,id=" + genID, row);
}

QString awf::AIWorkFlow::handleRef()
{
    next();  // 移动到当前 @ref 指令
    auto _this = getCurrentCommand();

    // 简单形式：@ref:some/path
    if (!_this.arg.isEmpty()) {
        return awf::readFileContents(getAbsPath(_this.arg));
    }

    // 复杂形式：带参数
    QString file = _this.getArg(RefArgsClass::toString(RefArgs::atFile));
    bool callLLM = (_this.getArg(RefArgsClass::toString(RefArgs::callLLM)) == "true");
    bool cacheAfterCallLLM = (_this.getArg(RefArgsClass::toString(RefArgs::cache)) == "true");
    QString targetSymbol = _this.getArg(RefArgsClass::toString(RefArgs::symbol));

    if (file.isEmpty()) {
        riseError("@ref 缺少 atFile 参数");
        return "";
    }

    Interpreter ip(ec);
    ip.loadFile(getAbsPath(file));

    bool foundRecord = false;
    QVector<QString> result;          // 收集目标符号的定义源代码
    QSet<QString> usedID;             // 记录已使用的 id

    // 遍历文件，寻找匹配的 record / genBegin
    for (size_t i = 0; i < ip.rowCount(); i++) {
        auto cur = ip.getCommandOf(i);

        // 收集所有已有的 id，避免生成冲突
        if (!cur.getArg(RecordArgsClass::toString(RecordArgs::id)).isEmpty()) {
            usedID.insert(cur.getArg(RecordArgsClass::toString(RecordArgs::id)));
        }

        if (cur.type == MP::record || cur.type == MP::genBegin) {
            if (cur.getArg(RecordArgsClass::toString(RecordArgs::symbolName)) == targetSymbol) {
                foundRecord = true;
                // symbolBegin 未使用，可保留或删除；这里保留原样
                // int symbolBegin = i;

                // 内层循环收集定义体
                bool foundEnd = false;
                for (size_t j = i + 1; j < ip.rowCount(); j++) {
                    auto innerCur = ip.getCommandOf(j);

                    if (!innerCur.getArg(RecordArgsClass::toString(RecordArgs::id)).isEmpty()) {
                        usedID.insert(innerCur.getArg(RecordArgsClass::toString(RecordArgs::id)));
                    }

                    // 找到匹配的结束标签
                    if ((innerCur.type == MP::recordEnd || innerCur.type == MP::genEnd) &&
                        innerCur.getArg(RecordArgsClass::toString(RecordArgs::id)) ==
                        cur.getArg(RecordArgsClass::toString(RecordArgs::id)))
                    {
                        foundEnd = true;
                        // symbolEnd 未使用，保留原样
                        // int symbolEnd = j;
                        break;  // ← 修正1：立即停止收集
                    }
                    else {
                        result.append(ip.getSource(j));
                    }
                }

                if (!foundEnd) {
                    riseWarn("文件末尾未闭合的 record/genBegin 标签，无法获取完整 ref 信息");
                }

                break;  // ← 修正2：只处理第一个匹配的符号
            }
        }
    }

    // 如果未找到定义且允许调用 LLM
    if (!foundRecord && callLLM) {
        // 读取文件并添加行号
        QVector<QString> fileContent = readFileContents(getAbsPath(file)).split("\n");
        for (int i = 0; i < fileContent.size(); i++) {
            // 修正3：使用 prepend 替代 push_front
            fileContent[i].prepend(QString("[line %1]").arg(i));
        }

        // 调用 LLM 定位符号
        QString aiResponse = aic.getGen({
            {system, QString(
                "在给定文本中,找出符号 \"%1\" 的定义位置的开始和结束行,,"
                "格式为posBegin{beginLine,endLine}posEnd,"
                "例如在 [line0]class ClassA{[line1]int a=0; [line2]}中获取ClassA的定义位置,"
                "你应当回复:\"posBegin{0,2}posEnd\","
                "如有多个定义,或符号名有歧义,也可给出多个范围,以逗号分隔,"
                "如\"posBegin{12,56},{78,92}posEnd\""
            ).arg(targetSymbol)},
            {system, QString("文本为:\n") + fileContent.join("\n")}
            });

        // 提取 AI 返回的行号范围
        auto dataMayBe = extract("posBegin", "posEnd", aiResponse.split("\n"));
        auto beginAndEndLines = extractDecimalNumbers(dataMayBe.join("\n"));

        // 修正4：优先级与逻辑修复
        if (beginAndEndLines.size() % 2 != 0) {
            beginAndEndLines.pop_back();
            cacheAfterCallLLM = false;
            riseWarn("获取到奇数个行号，存在未匹配的开始行和结束行");
        }

        LineBaseFileManager lfm(getAbsPath(file));
        for (size_t i = 0; i < beginAndEndLines.size(); i += 2) {
            int begin = beginAndEndLines[i];
            int end = beginAndEndLines[i + 1];

            // 修正5：begin > end 才报错
            if (begin > end) {
                riseWarn("开始行大于结束行");
                cacheAfterCallLLM = false;
                continue;
            }

            // 收集定义源代码
            for (int k = begin; k <= end; k++) {
                result.append(ip.getSource(k));
            }
            result.append("//结束-------------------------");

            // 如果需要缓存，在定义开始行之前插入 @record 标签
            if (cacheAfterCallLLM) {
                // 寻找第一个未使用的 id
                int newId = 0;
                while (usedID.contains(QString::number(newId))) {
                    newId++;
                }
                usedID.insert(QString::number(newId));  // 标记已用

                // 插入 @record 标签（原样保留枚举转换，补全括号）
                // 拿不准：operatorTypeToString 返回的字符串格式未知，假设为 "record"
                lfm.insertBeforeLineOfOrigin(begin,
                    QString("@%1,%2=%3,%4=%5")
                    .arg(operatorTypeToString(M_OperatorType::record))        // 原代码缺右括号，已补
                    .arg(RecordArgsClass::toString(RecordArgs::symbolName))
                    .arg(targetSymbol)
                    .arg(RecordArgsClass::toString(RecordArgs::id))
                    .arg(QString::number(newId))
                );
                // 注意：此处原逻辑只插入了一个标签，如需多个定义则需调整，保持原设计
            }
        }

        if (cacheAfterCallLLM) {
            lfm.writeBack();
        }
    }

    // 修正6：返回收集到的源代码
    return result.join("\n");
}

void awf::AIWorkFlow::launch(const QString& filePath, const QString& outPath) {
    fileProcesser.loadFile(getAbsPath(filePath));
    aic.setBase("https://api.deepseek.com", getFirstLine("E:\\cpp\\qt\\CPHe\\key.txt"), "deepseek-v4-flash", AIClient::deepSeek);
    aic.set_deepSeek_thinking(true);
    newFileCommand();

    if (!hasError())
        replaceWithFileBufferAndBackup(getAbsPath(filePath), getAbsPath(outPath));
    
}


void AIWorkFlow::replaceWithFileBufferAndBackup(const QString& filePath, const QString& outPath)
{
    QString absSrcPath = getAbsPath(filePath);
    qDebug() << "源文件路径:" << absSrcPath;
    qDebug().noquote() << "准备写入的内容：\n" << fileBuffer;

    // 确定最终写入的目标路径
    QString targetPath;
    bool useBackupMode = false;   // 是否使用备份模式（覆盖原文件并备份）

    if (outPath.isEmpty() || getAbsPath(outPath) == absSrcPath) {
        // 没有指定输出文件，或输出文件就是原文件本身 → 使用备份模式
        targetPath = absSrcPath;
        useBackupMode = true;
    }
    else {
        // 指定了不同的输出文件 → 直接写入，不备份
        targetPath = getAbsPath(outPath);
        useBackupMode = false;
    }

    // ---- 备份模式处理 ----
    if (useBackupMode) {
        // 若原文件存在则创建 .bak 备份；若不存在则无需备份（直接写入即可）
        if (QFile::exists(absSrcPath)) {
            QString backupPath = absSrcPath + ".bak";
            if (!QFile::copy(absSrcPath, backupPath)) {
                riseError("无法创建备份文件：" + backupPath);
                return;
            }
            qDebug() << "已创建备份:" << backupPath;
        }
        else {
            qDebug() << "源文件不存在，跳过备份，将直接创建新文件:" << absSrcPath;
        }
    }

    // ---- 写入内容到目标文件 ----
    // 确保目标文件所在目录存在（仅当输出路径与原路径不同时，可能目录未创建）
    QFileInfo targetInfo(targetPath);
    QDir targetDir = targetInfo.absoluteDir();
    if (!targetDir.exists()) {
        if (!targetDir.mkpath(".")) {
            riseError("无法创建目标文件所在目录：" + targetDir.absolutePath());
            return;
        }
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        riseError("无法打开目标文件写入：" + targetPath);
        return;
    }

    QTextStream out(&file);
    out << fileBuffer;
    file.close();

    qDebug() << "成功写入文件:" << targetPath;
    // 可在此添加文件大小校验等扩展逻辑
}