# 将 code 目录下的源文件和头文件添加到主目标
target_sources(${PROJECT_NAME} PRIVATE
    testclass.cpp
    testclass.h
    CodeAnalyzer.h
    CodeAnalyzer.cpp
    "CppCodeVisitor.cpp"
    "helpfulTypes.h" 
    "CppCodeVisitor.h" 
    "tools.h" 
    "tools.cpp" 
    "CodeFileReader.h" 
    "CodeFileReader.cpp" 
    "CppCodeFileReader.h" 
    "CppCodeFileReader.cpp" 
    "libClangContext.h" 
    "libClangContext.cpp" 
    "NameTree.h" 
    "NameTree.cpp" 
    "BaseCodeVisitor.h" 
    "BaseCodeVisitor.cpp")