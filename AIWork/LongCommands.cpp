#pragma once
#include"LongCommands.h"
#include"AIWorkFlow.h"
#include"BaseTool.h"
using namespace awf;
void FillCommand::onLaunch() {
    beginRow = aiwf->getCurrentIndex(); 
    genId = aiwf->getGenId();
}
void FillCommand::onFinish() {
    int endRow = aiwf->getCurrentIndex();
    QVector<ChatMessage> prompt;
    prompt.append({ Role::system, "编写的代码务必使用```cpp和```包裹" });
    prompt.append(promote);
    auto rawReply = aiwf->aic.getGen(prompt, genFunc);
    auto codeLines = extractLineBase("```cpp", "```", rawReply.split("\n"));
    aiwf->fileManeger.removeFromTo(beginRow, endRow);
    aiwf->index = beginRow - 1;
    aiwf->writeComment("@genBegin,id=" + genId);
    for (const QString& line : codeLines) {
        aiwf->writeSource(line);
    }
    aiwf->writeComment("@genEnd,id=" + genId);
}
void ChatCommand::onFinish() {
    // 直接使用搜集到的所有消息作为对话
    auto reply = aiwf->aic.getGen(promote);
    emit aiwf->outPut(reply);
}
void PrintCommand::onFinish() {
    // 输出所有 prompt 内容
    for (const auto& msg : promote) {
        emit aiwf->outPut(
            QString("[%1]: %2").arg(roleToString(msg.role), msg.message));
    }
}
