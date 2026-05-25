//#include "gui/CPHeMain.h"
#include <QtWidgets/QApplication>
//#include "code/CodeAnalyzer.h"
//#include"gui/NameMapView.h"
#include"AIWork/AIWorkFlow.h"
#include<qdebug.h>
#include<iostream>
#include"AIWork/BaseTool.h"
#include <QFile>
#include <QTextStream>
#include <QString>

int main(int argc, char* argv[])
{
    //todo 添加一个new scope 操作

    QApplication app(argc, argv);
    //awf::ExceptionCollector ec;
    //awf::ClangTool ct(ec);
    //awf::AIClient aic(ec);
    //aic.setBase("https://api.deepseek.com", getFirstLine("E:\\cpp\\qt\\CPHe\\key.txt"), "deepseek-v4-flash", awf::AIClient::deepSeek);
    //aic.set_deepSeek_thinking(false);
    //auto raw = aic.getGen({ {awf::user,"写一个c++函数,接收vector<int>值类型,返回排序后的vector<int>副本,函数名为sortAndRet,只写一个函数,不要写其他任何东西"} });
    //auto fragment = awf::extractLineBase("```cpp", "```",raw.split("\n"));
   
    //qDebug().noquote() << raw;
    //qDebug()<<"提取";

    //for (auto v:fragment)
    //{
    //    qDebug().noquote() << ct.getSymbolDef(v, "sortAndRet").join("\n");
    //}
    
    //qDebug().noquote() << reply;
    //QObject::connect(reply.get(), &awf::AITask::deltaReceived, [](const QString& data) {
    //    qDebug().noquote()<< data.toStdString();
    //});
    awf::ExceptionCollector ec;
    awf::Interpreter ip(ec);
    ip.loadFile("E:\\cpp\\qt\\CPHe\\AIWork\\des.cpp");
    for (size_t i = 0; i < ip.rowCount(); i++)
    {
        qDebug().noquote() << ip.getCommandOf(i).toString();
    }



    awf::AIWorkFlow af("E:\\cpp\\qt\\CPHe\\AIWork");
    af.setWrite(false);
    af.launch("des.cpp","");
    
    af.ec.printAll();
    //awf::Interpreter ip(ec);
    //ip.loadFile("E:\\cpp\\qt\\CPHe\\demo.cpp");
    //for (size_t i = 0; i < ip.rowCount(); i++)
    //{
    //    auto cmd = ip.getCommandOf(i);
    //    qDebug()<<cmd.toString();
    //}
    //ec.printAll();
    //

    return app.exec();
}