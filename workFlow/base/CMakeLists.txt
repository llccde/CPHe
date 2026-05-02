# 将 gui 目录下的源文件、头文件和 UI 文件添加到主目标
target_include_directories(${PROJECT_NAME} PRIVATE 

)
target_sources(${PROJECT_NAME} PRIVATE
baseOperator.h
Intent.h
Intent.cpp
workFlowContext.h
 "BaseSubContext.h" 
 "WorkFlowController.h" 
 "TaskManager.h" 
 "TaskFactory.h" 
 "TaskManager.cpp" 
 "workFlowContext.cpp")