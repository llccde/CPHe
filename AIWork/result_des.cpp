/*
	@fill{
	@ref FileManager.cpp
	@ref FileManaget.h
	@msg 写一个这个类的使用示例
	@copyPrompt
	@}
	@genBegin,op = fill,id = genID
*/
#include "FileManager.h"
#include <QDebug>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	// 1. 从已有的文本文件创建
	awf::LineBaseFileManager manager("D:/test.txt");

	// 2. 查看原始行数
	qDebug() << "原始行数:" << manager.originalLineCount();

	// 3. 在原始第2行之后插入一行
	manager.insertAfterLineOfOrigin(2, "这是在第2行之后插入的内容");
	
	// 4. 在原始第0行之前插入一行（即文件开头）
	manager.insertBeforeLineOfOrigin(0, "这是文件开头插入的内容");
	
	// 5. 删除原始的第1行到第2行
	manager.removeFromTo(1, 2);
	
	// 6. 获取修改后的完整内容
	QStringList content = manager.getContent();
	for (int i = 0; i < content.size(); ++i) {
		qDebug() << "行" << i << ":" << content[i];
	}
	
	// 7. 写回原文件
	if (manager.writeBack()) {
		qDebug() << "写回成功!";
	}
	
	// 8. 或者写入到另一个文件
	manager.writeTo("D:/test_output.txt");

	return 0;
}
/*
	@genEnd,id=0
*/