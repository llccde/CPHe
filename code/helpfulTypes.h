#pragma once
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
using ContentRes = std::unique_ptr<CharWrapper>;

using uniqueCharArray = std::unique_ptr<char[]>;
inline uniqueCharArray charArrayFromQString(const QString s) {
	QByteArray ba = s.toUtf8();
	auto data = std::make_unique<char[]>(ba.size() + 1);
	std::strcpy(data.get(), ba.constData());
	return data;
};