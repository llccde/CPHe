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
        case MP::print: {
            printCommand();
        }

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
    next();
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
            case MP::refFiles: {
                auto data = handleRefFile();
                for (auto& i : data)
                {
                    promot.append({ system,QString("文件%1:\n%2").arg(getRelativePath(i.absPath)).arg(i.content) });
                }
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
            case MP::copyPrompt:{
                justNextCommand();
                QVector<QString> data;
                for (auto& i:promot)
                {
                    data.append(i.toString());
                }
                data.append(userMessage.toString());
                QApplication::clipboard()->setText(data.join("\n"));
                break;
            }
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
    writeComment("@genBegin,"+args.join(","));
    // 注意：原代码中使用了未定义的 'user'，此处保持原样
    auto data = extractLineBase("```cpp", "```", aic.getGen(promot, genFunc).split("\n"));
    for (auto&s:data)
    {
        writeSource(s);
    }
    
    writeComment("@genEnd,id=" + genID);
}

void awf::AIWorkFlow::chatCommand() {
    next();
    int row = getCurrentIndex();
    auto cur = getCurrentCommand();

    QVector<ChatMessage> promot;
    ChatMessage userMessage = { user, "" };
    userMessage.message.append(cur.arg);
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
                promot.append({ user, data });
                break;
            }
            case MP::msg:
            case MP::normalComment: {
                auto text = justNextCommand();
                userMessage.message.append(text.arg);
                break;
            }
            case MP::end:
                findEnd = true;
                justNextCommand();
                break;
            case MP::copyPrompt: {
                justNextCommand();
                QVector<QString> data;
                for (auto& i : promot)
                {
                    data.append(i.toString());
                }
                data.append(userMessage.toString());
                QApplication::clipboard()->setText(data.join("\n"));
                break;
            }
            case MP::refFiles: {
                auto data = handleRefFile();
                for (auto&i:data)
                {
                    promot.append({ system,QString("文件%1:\n%2").arg(getRelativePath(i.absPath)).arg(i.content)});
                }
                break;
            }
            default:
                riseWarn("在@chat 长指令标记 区间内,除去@ref,@end,不支持任何其他指令");
                justNextCommand();
                break;
            }
        }
    }
    promot.append(userMessage);
    auto data =aic.getGen(promot);
    emit outPut(data);
}
void awf::AIWorkFlow::printCommand()
{
    next();
    int row = getCurrentIndex();
    auto cur = getCurrentCommand();
    if (!cur.isLongOperator) {
        emit outPut(cur.arg);
        return;
    }
    QVector<ChatMessage> promot;
    ChatMessage userMessage = { user, "" };
    userMessage.message.append(cur.arg);
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
                promot.append({ user, data });
                break;
            }
            case MP::msg:
            case MP::normalComment: {
                auto text = justNextCommand();
                userMessage.message.append(text.arg);
                break;
            }
            case MP::end:
                findEnd = true;
                justNextCommand();
                break;
            case MP::copyPrompt: {
                justNextCommand();
                QVector<QString> data;
                for (auto& i : promot)
                {
                    data.append(i.toString());
                }
                data.append(userMessage.toString());
                QApplication::clipboard()->setText(data.join("\n"));
                break;
            }
            case MP::refFiles: {
                auto data = handleRefFile();
                for (auto& i : data)
                {
                    promot.append({ system,QString("文件%1:\n%2").arg(getRelativePath(i.absPath)).arg(i.content) });
                }
                break;
            }
            default:
                riseWarn("在@print 长指令标记 区间内,除去@ref,@end,不支持任何其他指令");
                justNextCommand();
                break;
            }
        }
    }
    promot.append(userMessage);
    for (auto&i:promot)
    {
        emit outPut(QString("%1:\n%2").arg(roleToString(i.role)).arg(i.message));
    }


}
QString awf::AIWorkFlow::handleRef()
{
    next();  // 移动到当前 @ref 指令
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
    next();  // 移动到当前 @refFile 指令
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



void awf::AIWorkFlow::launch(const QString& filePath, const QString& outPath) {
    prepareLaunch(filePath, outPath, -1);
    
    newFileCommand();

    if (!hasError()&&doWrite)
        replaceWithFileBufferAndBackup(getAbsPath(filePath), getAbsPath(outPath));
    
}

void awf::AIWorkFlow::prepareLaunch(const QString& filePath, const QString& outPath, int beginRow)
{
    fileProcesser.loadFile(getAbsPath(filePath));
    aic.setBase("https://api.deepseek.com", getFirstLine("E:\\cpp\\qt\\CPHe\\key.txt"), "deepseek-v4-flash", AIClient::deepSeek);
    aic.set_deepSeek_thinking(false );
    fileManeger = LineBaseFileManager(filePath);
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
class Command{
    QHash<M_OperatorType, std::function<void(M_Command&)>> switchCase;
};