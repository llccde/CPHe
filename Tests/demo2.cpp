#pragma once
/*
@fill{
@nameFunc JieCheng
@msg 生成一个函数,接受一个数字N,返回其阶乘
*/
//@genBegin,op = fill,id = genID,symbolName = JieCheng
long long JieCheng(int N) {
    long long result = 1;
    for (int i = 1; i <= N; ++i) {
        result *= i;
    }
    return result;
}
//@genEnd,id=0
