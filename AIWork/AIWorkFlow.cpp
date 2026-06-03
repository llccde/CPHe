#include"AIWorkFlow.h"
#include "AIWorkFlow.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include"FileManager.h"
#include"BaseTool.h"
#include"qclipboard.h"
#include"qapplication.h"
#include<qdiriterator.h>
#include"LongCommands.h"
using namespace awf;
// AIWorkFlow.cpp
awf::AIWorkFlow::AIWorkFlow(const QString& working)
    : workingFolder(working), fileProcesser(ec), aic(ec), tool(ec)
{
}

QString awf::AIWorkFlow::getAbsPath(const QString& path) {
    return QDir(workingFolder).absoluteFilePath(path);
}

QString awf::AIWorkFlow::getRelativePath(const QString& path)
{
    return QDir(workingFolder).relativeFilePath(path);
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

void awf::AIWorkFlow::next() {
    index++;
}

bool awf::AIWorkFlow::hasNext() {
    return index + 1 < fileProcesser.rowCount();
}

bool awf::AIWorkFlow::isCurInCommentBlock()
{
    return fileProcesser.isCommentBlockAfter(getCurrentIndex());
}

void awf::AIWorkFlow::writeFile(const QString& data, int index) {
    auto clean = tool.clearSingleLineBreak(data);
    if (index != -1 && index < fileProcesser.rowCount()) {
        auto data_f = tool.SameTab(clean, fileProcesser.getSource(index));
        fileManeger.insertAfterLineOfOrigin(index,data_f);
        return;
    }
    fileManeger.insertAfterLineOfOrigin(getCurrentIndex(), clean);
}

void awf::AIWorkFlow::writeComment(const QString& data) {
    if (data.count("\n")) {
        QRegularExpression re(R"(\r\n|\n|\r)");
        writeComment(data.split(re));
    }
    if (!fileProcesser.isCommentBlockAfter(getCurrentIndex())) {
        writeFile("//" + data, index);
    }
    else {
        writeFile(data, index);
    }
}

void awf::AIWorkFlow::writeComment(const QVector<QString> data) {
    if (!isCurInCommentBlock()) {
        auto dataC = data;
        tool.clearLineBreak(dataC);
        if (index != -1 && index < fileProcesser.rowCount()) {
            tool.FormatTab(dataC, fileProcesser.getSource(index));
        }
        fileManeger.insertAfterLineOfOrigin(getCurrentIndex(), "/*" + dataC.join("\n") + "*/");
    }
    else {
        writeFile(data, index);
    }
}

void awf::AIWorkFlow::writeSource(const QString& data) {
    QRegularExpression re(R"(\r\n|\n|\r)");
    writeSource(data.split(re));
}

void awf::AIWorkFlow::writeSource(const QVector<QString> data) {
    if (isCurInCommentBlock()) {
        auto dataC = data;
        dataC.push_front("*/");
        dataC.push_back("/*");
        tool.clearLineBreak(dataC);
        if (index != -1 && index < fileProcesser.rowCount()) {
            int start = fileProcesser.getCommentStartOfCommentRow(getCurrentIndex());
            if (start == -1) start = index;
            tool.FormatTab(dataC, fileProcesser.getSource(start));
        }
        fileManeger.insertAfterLineOfOrigin(getCurrentIndex(), dataC.join("\n"));
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
    fileManeger.insertAfterLineOfOrigin(getCurrentIndex(), dataC.join("\n"));
}

void awf::AIWorkFlow::riseWarn(const QString& wrn) {
    ec.Wrn(wrn);
}

void awf::AIWorkFlow::riseError(const QString& err) {
    ec.Err(err);
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


void AIWorkFlow::newMainLoop() {
    while (hasNext() && !ec.hasErr()) {
        next();
        auto cur = getCurrentCommand();
        // ---- 处理长指令（isLongOperator 为真） ----
        if (cur.isLongOperator) {
            std::unique_ptr<RuntimeCommand> rc;
            switch (cur.type) {
            case MP::fill:  rc = std::make_unique<FillCommand>();  break;
            case MP::chat:  rc = std::make_unique<ChatCommand>();  break;
            case MP::print: rc = std::make_unique<PrintCommand>(); break;
            default:
                ec.Err(QString("不支持的长指令类型: %1")
                    .arg(operatorTypeToString(cur.type)));
                continue;
            }
            rc->aiwf = this;
            rc->ec = &ec;
            rc->command = cur;
            rc->onLaunch();
            commandStack.push(std::move(rc));
            continue;
        }
        // ---- 处理普通指令 ----
        // 1) 根节点允许的特殊指令（栈为空）
        if (commandStack.empty()) {
            switch (cur.type) {
            case MP::genBegin:
                handleGenLable();
                break;
            case MP::genEnd:
                ec.Err("未匹配的 @genEnd 标签");
                break;
            case MP::debugger:
                debugger();
                break;
            case MP::print:
                // 非长指令的 @print 直接输出内容
                emit outPut(cur.arg);
                break;
            case MP::normalComment:
                break;
            default:
                ec.Wrn(QString("根节点下不支持指令 %1")
                    .arg(operatorTypeToString(cur.type)));
                break;
            }
            continue;
        }
        // 2) 栈非空 → 交给栈顶命令处理
        auto& top = commandStack.top();
        switch (cur.type) {
        case MP::ref: {
            QString content = handleRef();            // 只获取内容
            top->onReciveFileContent(content, cur.arg, Role::system);
            break;
        }
        case MP::refFiles: {
            QVector<FileBuffer> files = handleRefFile();
            for (const auto& f : files) {
                top->onReciveFileContent(f.content, getRelativePath(f.absPath), Role::system);
            }
            break;
        }
        case MP::nameFunc:
            top->onName(SymbolType::Func, cur.arg);
            break;
        case MP::moduleName:
            top->onModuleName(cur.arg);
            break;
        case MP::msg:
        case MP::normalComment:
            top->onMessage(cur.arg);
            break;
        case MP::copyPrompt:
            top->onCopyPrompt();
            break;
        case MP::end: {
            top->onFinish();
            commandStack.pop();
            break;
        }
        default:
            ec.Wrn(QString("长指令内部暂不支持的指令: %1")
                .arg(operatorTypeToString(cur.type)));
            break;
        }
    }
    // 文件结束时栈仍未空 → 缺少 @end
    if (!commandStack.empty()) {
        QStringList names;
        while (!commandStack.empty()) {
            names << operatorTypeToString(commandStack.top()->command.type);
            commandStack.pop();
        }
        ec.Err(QString("文件结尾处未闭合的长指令:\n%1").arg(names.join("\n")));
    }
}

void awf::AIWorkFlow::debugger() {
    auto current = getCurrentCommand();
    auto next = peekNext();
    (void)current; (void)next; // 避免未使用变量警告
    return;
}

void awf::AIWorkFlow::handleGenLable() {
    int begin = getCurrentIndex();
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
            break;
        case MP::genEnd:
        {
            if (next.getArg("id") == _this.getArg("id")) {
                findEnd = true;
            }
            justNextCommand();
            int end = getCurrentIndex();
            fileManeger.removeFromTo(begin, end);

            break;
        }
        default:
            justNextCommand();
        }
        if (findEnd) break;
    }
}

QString awf::AIWorkFlow::handleRef()
{
    auto _this = getCurrentCommand();
    assert(_this.type == MP::ref);
    // 简单形式：@ref:some/path
    if (!_this.arg.isEmpty()) {
        return awf::readFileContents(getAbsPath(_this.arg));
    }

    // 复杂形式：带参数
    QString file = _this.getArg(ArgsClass::toString(Args::file));
    bool callLLM = _this.contains(ArgsClass::toString(Args::callLLM))&& _this.getArg(ArgsClass::toString(Args::callLLM))!="false";
    bool cacheAfterCallLLM = _this.contains(ArgsClass::toString(Args::cache))&& _this.getArg(ArgsClass::toString(Args::cache))!="false";
    QString targetSymbol = _this.getArg(ArgsClass::toString(Args::symbol));
    QString desMsg = _this.getArg(ArgsClass::toString(Args::msg));
    if (cacheAfterCallLLM && !callLLM)callLLM = true;
    if (file.isEmpty()) {
        riseError("@ref 缺少 file 参数");
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
        if (!cur.getArg(ArgsClass::toString(Args::id)).isEmpty()) {
            usedID.insert(cur.getArg(ArgsClass::toString(Args::id)));
        }

        if (cur.type == MP::record || cur.type == MP::genBegin) {
            if (cur.getArg(ArgsClass::toString(Args::symbol)) == targetSymbol) {
                foundRecord = true;
                // symbolBegin 未使用，可保留或删除；这里保留原样
                // int symbolBegin = i;

                // 内层循环收集定义体
                bool foundEnd = false;
                size_t j = i + 1;
                for (; j < ip.rowCount(); j++) {
                    auto innerCur = ip.getCommandOf(j);

                    if (!innerCur.getArg(ArgsClass::toString(Args::id)).isEmpty()) {
                        usedID.insert(innerCur.getArg(ArgsClass::toString(Args::id)));
                    }

                    // 找到匹配的结束标签
                    if ((innerCur.type == MP::recordEnd || innerCur.type == MP::genEnd) &&
                        innerCur.getArg(ArgsClass::toString(Args::id)) ==
                        cur.getArg(ArgsClass::toString(Args::id)))
                    {
                        foundEnd = true;
                        // symbolEnd 未使用，保留原样
                        // int symbolEnd = j;
                        break;  // ← 修正1：立即停止收集
                    }
                    else {
                        if(!ip.isCommandComment(j))result.append(ip.getSource(j));
                    }
                }

                if (!foundEnd) {
                    riseWarn("文件末尾未闭合的 record/genBegin 标签，无法获取完整 ref 信息");
                    break;
                }
                else
                {
                    result.append("//结束-------------------------");
                    i = j;
                }
                
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
                "在给定文本中,找出符号 \"%1\" 的定义位置的开始和结束行,"
                "%2"
                "格式为posBegin{beginLine,endLine}posEnd,"
                "例如在 [line0]class ClassA{[line1]int a=0; [line2]}中获取ClassA的定义位置,"
                "你应当回复:\"posBegin{0,2}posEnd\","
                "如有多个定义,或符号名有歧义,也可给出多个范围,以逗号分隔,"
                "如\"posBegin{12,56},{78,92}posEnd\""
            ).arg(targetSymbol).arg(desMsg)}, 
            {system, QString("文本为:\n") + fileContent.join("\n")}
            });

        // 提取 AI 返回的行号范围
        auto dataMayBe = extractContent("posBegin", "posEnd", aiResponse);
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
                if (!ip.isCommandComment(k))result.append(ip.getSource(k));
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

                // 插入 @record 标签
                lfm.insertBeforeLineOfOrigin(begin,
                    QString("//@%1,%2=%3,%4=%5")
                    .arg(operatorTypeToString(M_OperatorType::record))
                    .arg(ArgsClass::toString(Args::symbol))
                    .arg(targetSymbol)
                    .arg(ArgsClass::toString(Args::id))
                    .arg(QString::number(newId))
                );
                lfm.insertAfterLineOfOrigin(end,
                    QString("//@%1,%2=%3")
                    .arg(operatorTypeToString(M_OperatorType::end))
                    .arg(ArgsClass::toString(Args::id))
                    .arg(QString::number(newId))
                );
            }
        }

        if (cacheAfterCallLLM) {
            lfm.writeBack();
        }
    }


    return result.join("\n");
}

QVector<FileBuffer> awf::AIWorkFlow::handleRefFile()
{
    auto _this = getCurrentCommand();

    // ---------- 解析参数 ----------
    QString endWith = _this.getArg(Args::endWith);
    QString subDirStr = _this.getArg(Args::subDir);
    QString baseFolder = _this.getArg(Args::baseFolder);

    // 是否递归子目录（"true" 或 "1" 视为真）
    bool recursive = (subDirStr.toLower() == "true" || subDirStr == "1");

    // 确定搜索根目录
    QString baseDirPath = baseFolder.isEmpty() ? workingFolder
        : getAbsPath(baseFolder);
    QDir baseDir(baseDirPath);
    if (!baseDir.exists()) {
        riseError(QString("@refFile 指定的 baseFolder 不存在: %1").arg(baseDirPath));
        return {};
    }

    // 构建文件名过滤器
    QStringList nameFilters;
    if (!endWith.isEmpty()) {
        const QStringList exts = endWith.split('|', Qt::SkipEmptyParts);
        for (const QString& ext : exts) {
            QString trimmed = ext.trimmed();
            if (!trimmed.isEmpty())
                nameFilters << QStringLiteral("*.%1").arg(trimmed);
        }
    }

    // ---------- 收集文件 ----------
    QVector<FileBuffer> result;          // 每个元素存放一个文件的完整内容

    if (recursive) {
        QDirIterator it(baseDirPath, nameFilters, QDir::Files,
            QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QString content = awf::readFileContents(it.filePath());
            if (!content.isEmpty())   // 避免加入完全为空的文件（可按需求调整）
                result.append({it.filePath() ,content});
        }
    }
    else {
        baseDir.setNameFilters(nameFilters);
        const QStringList files = baseDir.entryList(QDir::Files);
        for (const QString& fileName : files) {
            QString fullPath = baseDir.absoluteFilePath(fileName);
            QString content = awf::readFileContents(fullPath);
            if (!content.isEmpty())
                result.append({ fullPath,content });
        }
    }

    // 如果最终未读取到任何文件，给出警告但不算致命错误
    if (result.isEmpty()) {
        riseWarn(QString("@refFile 未找到匹配的文件 (baseFolder=%1, endWith=%2)")
            .arg(baseDirPath, endWith));
    }

    return result;
}


void AIWorkFlow::launch(const QString& filePath, const QString& outPath) {
    prepareLaunch(filePath, outPath, -1);
    newMainLoop();   
    if (!hasError() && doWrite)
        replaceWithFileBufferAndBackup(getAbsPath(filePath), getAbsPath(outPath));
}

void awf::AIWorkFlow::prepareLaunch(const QString& filePath, const QString& outPath, int beginRow)
{
    fileProcesser.loadFile(getAbsPath(filePath));
    aic.setBase("https://api.deepseek.com", getFirstLine("E:\\cpp\\qt\\CPHe\\key.txt"), "deepseek-v4-flash", AIClient::deepSeek);
    aic.set_deepSeek_thinking(false );
    fileManeger = LineBaseFileManager(getAbsPath(filePath));
    index = beginRow;
}


void AIWorkFlow::replaceWithFileBufferAndBackup(const QString& filePath, const QString& outPath)
{
    QString absSrcPath = getAbsPath(filePath);
    qDebug() << "源文件路径:" << absSrcPath;
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
            if (QFile::exists(backupPath))
                QFile::remove(backupPath);  // 删除旧备份
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

    fileManeger.writeTo(targetPath);
}   
