cmake_minimum_required(VERSION 3.16)
project(CPHe LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)
qt_standard_project_setup()

# LLVM 支持
list(APPEND CMAKE_PREFIX_PATH "E:/LIBS/LLVM/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

# 添加 clang 支持（通常 clang 的 cmake 文件在同一 LLVM 安装目录下）
list(APPEND CMAKE_PREFIX_PATH "E:/LIBS/LLVM/lib/cmake/clang")
find_package(Clang REQUIRED CONFIG)



# 根目录源文件
set(ROOT_SOURCES
    main.cpp
    CPHe.rc
)

# 创建主可执行目标
qt_add_executable(${PROJECT_NAME} ${ROOT_SOURCES})

# 添加子目录
add_subdirectory(gui)
add_subdirectory(code)

# 目标属性
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE FALSE
)

# LLVM 和 clang 头文件
target_include_directories(${PROJECT_NAME} PRIVATE ${LLVM_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CLANG_INCLUDE_DIRS})

# 链接库（优先使用 CMake 目标）
if(TARGET LLVM::LLVM)
    target_link_libraries(${PROJECT_NAME} PRIVATE LLVM::LLVM)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBRARIES})
endif()

if(TARGET libclang)
    target_link_libraries(${PROJECT_NAME} PRIVATE libclang)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CLANG_LIBRARIES})
endif()

# Qt 链接
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)