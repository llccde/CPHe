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
	LibClangContext* libClangContext;
public:
	CodeAnalyzer(LibClangContext*);

	void launch(BaseVisitor* visitor);

public:
	~CodeAnalyzer();
};

