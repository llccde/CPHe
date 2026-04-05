#pragma once
#include"helpfulTypes.h"
#include"qfileinfo.h"
class M_File {
public:
	virtual ContentRes getContent() = 0;
	QString getName() {
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

struct CXUnsavedFile;
typedef void* CXIndex;
typedef struct CXTranslationUnitImpl* CXTranslationUnit;

class LibClangContextResPack;
class LibClangIndexAndTranslationUnit {
public:

	UniqueResOf<CXIndex> index;
	UniqueResOf<CXTranslationUnit> tu;
	LibClangIndexAndTranslationUnit (LibClangIndexAndTranslationUnit&& other) noexcept;
	LibClangIndexAndTranslationUnit(LibClangContextResPack* data);
	LibClangIndexAndTranslationUnit() {};
};
class LibClangContextResPack {
public:
	int mainIndex;
	int fileNum;
	Vector<ContentRes> contents;
	Vector<uniqueCharArray> paths;
	std::unique_ptr<CXUnsavedFile[]> unsaveFilesCache;
	LibClangIndexAndTranslationUnit getUnit();
	LibClangContextResPack(int size,int mainIndex);

	~LibClangContextResPack();
};



class LibClangContext {
	
	M_File* main = nullptr;
	Vector<UniqueFilePtr> files;

public:
	enum fileType {
		isMainFile,
		notMainFile
	};
	void addFile(UniqueFilePtr file, fileType isMain = notMainFile) {
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
		for (int i = 0; i < files.size(); i++) {
			if (files[i].get() == main)
				return i;
		}
		assert(false);
		return 0;
	}
	int getFileNum() {
		return files.size();
	}
	std::unique_ptr<LibClangContextResPack> getFileCache();
};