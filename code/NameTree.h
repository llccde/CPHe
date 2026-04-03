#pragma once
#include"helpfulTypes.h"
#include"qvector.h"
class NameMapNode;
using UniqueNameMapPtr = std::unique_ptr<NameMapNode>;

class NameMapNode {

	Vector<UniqueNameMapPtr> children;
	QString myName;
	NameMapNode* parent = nullptr;
public:

	const enum status {
		normal,
		root,
		unAvailable
	} state = normal;
	bool available() { return state != unAvailable; }
	QString getMyName() {
		return myName;
	}
	int childrenNum() {
		return children.size();
	}
	void add(UniqueNameMapPtr&& p) {
		children.push_back(std::move(p));

	}
	void add(NameMapNode*&& node) {
		children.push_back(UniqueNameMapPtr(node));
	};

	NameMapNode* operator[](int index) {
		assert(index < children.size());
		return children[index].get();
	}
	Vector<NameMapNode*> getChildByName(const QString& name) {
		Vector<NameMapNode*> reslut;
		for (auto& i : children) {
			if (i->getMyName() == name) reslut.push_back(i.get());
		}
		return reslut;
	}
	NameMapNode(QString name = "", status state_ = normal, NameMapNode* parent = nullptr) :myName(name), state(state_), parent(parent) {
	}
	NameMapNode* getParent() {
		assert(parent != nullptr);
		return parent;
	}
	Vector<NameMapNode*> findNodeByNameSpaceCallOnRoot(QString path) {
		assert(state == root);
		Vector<NameMapNode*> result;
		QVector<QString> pathVec = path.split("::");
		for (auto& i : children) {
			i->findNodeByNameSpace_impl(pathVec, result);
		}
		return result;
	};

	void outputNameMap(int level = 0);
	virtual CodePosition getPosition() = 0;
protected:
	virtual void findNodeByNameSpace_impl(QVector<QString> path, Vector<NameMapNode*>& result) {
		if (path.size() > 0) {
			if (path.size() == 1) {
				if (this->myName == path[0]) {
					result.push_back(this);
					return;
				}else{
					return;
				}
			}else if(path.size() >= 1){
				if (this->myName == path[0]) {
					path.pop_front();
					for (auto& i : children) {
						i->findNodeByNameSpace_impl(path, result);
					}
				}else{
					return;
				}
			}
		}else{
			assert(false);
			return;
		}
	}

};
using NameMap = NameMapNode;