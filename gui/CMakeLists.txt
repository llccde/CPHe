# 将 gui 目录下的源文件、头文件和 UI 文件添加到主目标
target_include_directories(${PROJECT_NAME} PRIVATE 
../GUIInterface/
)
target_sources(${PROJECT_NAME} PRIVATE
    CPHeMain.cpp
    CPHeMain.h
    CPHeMain.ui
    NameMapView.cpp
    NameMapView.h
    NameMapView.ui

 "HighlightDelegate.h"
"HighlightDelegate.cpp"  )