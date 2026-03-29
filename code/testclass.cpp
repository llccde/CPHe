namespace myName {
	class myClass;
}
namespace  myName{
	class myClass {
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
	};
}
