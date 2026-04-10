#include "CodeAnalyzer.h"
#include <clang-c/Index.h>
#include<functional>
#include<iostream>
#include"helpfulTypes.h"
#include<qqueue.h>
#include"libClangContext.h"
#include"NameTree.h"

CodeAnalyzer::CodeAnalyzer(LibClangContext* context):
	libClangContext(context)
{
	
}

void CodeAnalyzer::launch(BaseVisitor* _visitor){
	auto cachePack = libClangContext->getFileCache();
	auto unit = cachePack->getUnit();
	CXCursor cursor = clang_getTranslationUnitCursor(*(unit.tu));
	clang_visitChildren(
		cursor,
		[](CXCursor cursor, CXCursor p2, CXClientData _this) {
			return static_cast<BaseVisitor*>(_this)->cursorVisitor(cursor, p2);
		},
		static_cast<void*>(_visitor)
	);
	
}

//void CodeAnalyzer::outPutNameMap()
//{
//	this->nameMap->outputNameMap();
//}
//CodeAnalyzer::NameData CodeAnalyzer::getName(QVector<QString>& namePath)
//{
//	return NameData();
//}
//
//bool CodeAnalyzer::pathAvailable(QString path)
//{
//	return getNode(path).size()!=0;
//}
//
//Vector<NameMapNode*> CodeAnalyzer::getNode(QString path)
//{
//	Vector<NameMapNode*> result;
//	auto namePath = path.split("::");
//	struct task {
//		NameMapNode* node;
//		int level;
//	};
//	QQueue<task> queue;
//	queue.append({ nameMap.get(),-1 });
//	NameMapNode* currnetParent = nameMap.get();
//	while (!queue.empty()) {
//		auto i = queue.front();
//		queue.pop_front();
//		int nextLevel = i.level + 1;
//		if (nextLevel == namePath.size()) {
//			result.push_back(i.node);
//			continue;
//		};
//		QString& target = namePath[i.level + 1];
//		for (auto child : i.node->getChildByName(target)) {
//			queue.push_back({ child,nextLevel });
//		}
//	}
//	return result;
//}

CodeAnalyzer::~CodeAnalyzer()
{
}

