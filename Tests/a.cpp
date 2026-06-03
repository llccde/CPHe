/*

@fill{
	@msg 生成一个函数,接收两个QString,A,和B,从A,B当中轮流取字符,加到一个新字符串的末尾,直到一个字符串用尽,此时把剩下的字符串整个贴到末尾.返回最终的字符
	@nameFunc addCrossStr
	@ref,file=./Gui/
@}
@genBegin,id=0
*/
QString addCrossStr(const QString &A, const QString &B) {
	QString result;
	int minLen = qMin(A.length(), B.length());
	
	for (int i = 0; i < minLen; ++i) {
		result.append(A.at(i));
		result.append(B.at(i));
	}
	
	if (A.length() > minLen) {
		result.append(A.mid(minLen));
	} else if (B.length() > minLen) {
		result.append(B.mid(minLen));
	}
	
	return result;
}
/*
@genEnd,id=0
*/