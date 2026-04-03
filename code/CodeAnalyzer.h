#pragma once
#include <memory>
#include<qstring.h>
#include<QVector>
#include <cstring>
#include<qfileinfo.h>
#include<cassert>
#include"helpfulTypes.h"
#include<iostream>
#include"BaseCodeVisitor.h"
class LibClangContext;

class CodeAnalyzer {
public:
private:
	std::unique_ptr<LibClangContext> libClangContext;
public:
	CodeAnalyzer(std::unique_ptr<LibClangContext>);
	LibClangContext* getLibClangContext() { return libClangContext.get();}

	void launch(BaseVisitor* visitor);

public:
	~CodeAnalyzer();
};

