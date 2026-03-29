# 将 code 目录下的源文件和头文件添加到主目标
target_sources(${PROJECT_NAME} PRIVATE
    testclass.cpp
    testclass.h
    CodeAnalyzer.h
    CodeAnalyzer.cpp
    "helpfulTypes.h" 
    "codeVisitor.h")