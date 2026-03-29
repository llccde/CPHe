#pragma once
#include <memory>
#include<qstring.h>
#include<QVector>
#include <cstring>
#include<qfileinfo.h>
#include<cassert>
#include"helpfulTypes.h"
//应当保证每次调用get返回的字符串都是不变的

using my_size = unsigned long;

class CodeAnalyzer {
public:
	class M_File {
	public:
		virtual ContentRes getContent() = 0;
		virtual QString getName() {
			QFileInfo info(getFullPath());
			return info.fileName();
		};
		virtual QString getFullPath() = 0;
		uniqueCharArray getNameRaw() {
			return charArrayFromQString(getName());
		}
		uniqueCharArray getFullPathRaw() {
			return charArrayFromQString(getFullPath());
		}
		
		virtual bool isLocalFile() {
			return false;
		};
		virtual ~M_File() {
		};
	};

	using UniqueFilePtr = std::unique_ptr<M_File>;
	class NameMapNode;
	using UniqueNameMapPtr = std::unique_ptr<NameMapNode>;

	class NameMapNode {
		
		Vector<UniqueNameMapPtr> children;
		QString myName;
		static NameMapNode unAvailableNode;
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
		NameMapNode* operator[](int index){
			assert(index < children.size());
			return children[index].get();
		}
		NameMapNode* getChildByName(const QString& name) {
			for (auto& i : children) {
				if (i->getMyName() == name) return i.get();
			}
			return &unAvailableNode;
		}
		NameMapNode(QString name = "",status state_ = normal):myName(name),state(state_){
		}
	};
	using NameMap = NameMapNode;


private:
	M_File* main = nullptr;
	Vector<UniqueFilePtr> files;
	NameMap nameMap;
	
public:
	CodeAnalyzer();
	enum fileType {
		isMainFile,
		notMainFile
	};
	void addFile(UniqueFilePtr file,fileType isMain = notMainFile) {
		if (isMain == isMainFile) {
			assert(main == nullptr);
			main = file.get();
		}
		files.push_back(std::move(file));
	};
	ContentRes getFileContentByaPth(QString path) {
		for (auto& i : files) {
			if (i->getFullPath() == path) return i->getContent();
		}
		assert(false);
		return ContentRes();
	}
	ContentRes getFileContentByIndex(int i) {
		assert(i < files.size());
		return files[i]->getContent();
	}
	int getMainIndex() {
		assert(main != nullptr);
		for (int i = 0; i < files.size(); i++){
			if (files[i].get() == main) 
				return i;
		}
		assert(false);
		return 0;
	}
	void parseNames();
private:
	class Visitor;
	friend class Visitor;
	std::unique_ptr<Visitor> _visitor;
public:
	~CodeAnalyzer();
};
