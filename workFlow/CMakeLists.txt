# 将 gui 目录下的源文件、头文件和 UI 文件添加到主目标

target_include_directories(${PROJECT_NAME} PRIVATE 

)
#add_subdirectory(operators)
#add_subdirectory(forCpp)
#add_subdirectory(base)
#add_subdirectory(baseOpt)
target_sources(${PROJECT_NAME} PRIVATE
	
	PSCTranslater.h
 "PSCTranslater.cpp" "PSCErrorType.h" "PSCLangErr.h" "SubStr.h" "PSCInterpreter.h")