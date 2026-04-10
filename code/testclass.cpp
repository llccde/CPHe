//这是一个用来测试cpp解析的文件,为了方便编辑,放在了项目里面
namespace myName {
	class myClass;
}
class S {};
class myName::myClass:public S {
	int a;
	int* b = nullptr;
public:
	int func(int a) {
		class innerClass {
		public:
			int operator()(int value) {
				return value * value;
			}
		};
		return innerClass()(a);
	}
	int func2();
};
int myName::myClass::func2() { return 1; }
