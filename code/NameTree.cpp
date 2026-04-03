#include"NameTree.h"
#include<iostream>

void NameMapNode::outputNameMap(int level)
{
	for (size_t i = 0; i < level; i++) {
		std::cout << "   ";//三个空格一级
	}
	std::cout << myName.toStdString() << "\n";
	for (auto& i : children) {
		i->outputNameMap(level + 1);
	}
}