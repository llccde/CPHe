# 将 gui 目录下的源文件、头文件和 UI 文件添加到主目标

target_include_directories(${PROJECT_NAME} PRIVATE
../../code/
../base/
)
target_sources(${PROJECT_NAME} PRIVATE
ManyBaseOpts.h
ManyBaseOpts.cpp
 "BaseOptFactory.h" "BaseOptFactory.cpp") 