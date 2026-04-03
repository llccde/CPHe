#pragma once
#include<functional>
#include<memory>
#include<qbytearray.h>
#include<qstring.h>
template <class T>
using delType = std::function<void(T*)>;
template<class T>
using UniqueResOf = std::unique_ptr<T, delType<T>>;

using my_size = unsigned long;

template<class T>
using Vector = std::vector<T>;
class CharWrapper {
protected:
	const unsigned long _length;
public:

	CharWrapper(my_size l) :_length(l) {};
	const my_size length() { return _length; };
	virtual const char* getCharArray() = 0;

	virtual ~CharWrapper() {};
};
//某些资源,析构后不一定可以回收,例如静态数组
using ContentRes = std::unique_ptr<CharWrapper>;
//一定可以被回收的字符串
using uniqueCharArray = std::unique_ptr<char[]>;

inline uniqueCharArray charArrayFromQString(const QString s) {
	QByteArray ba = s.toUtf8();
	auto data = std::make_unique<char[]>(ba.size() + 1);
	std::strcpy(data.get(), ba.constData());
	return data;
};

struct CodePosition {
	QString file;
	unsigned rowBegin, columBegin, rowEnd, columnEnd;

	CodePosition(const QString& file, unsigned rowBegin, unsigned columBegin, unsigned rowEnd, unsigned columnEnd)
		: file(file), rowBegin(rowBegin), columBegin(columBegin), rowEnd(rowEnd), columnEnd(columnEnd){}
	CodePosition() {};
};