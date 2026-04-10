cmake_minimum_required(VERSION 3.16)
project(CPHe LANGUAGES CXX)
cmake_policy(SET CMP0077 NEW)
variable_watch(CMAKE_MSVC_RUNTIME_LIBRARY)
include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)
qt_standard_project_setup()

# 添加 LLVM 和 Clang 的 CMake 路径
if(WIN32)
    # Windows 下添加自定义 LLVM 和 Clang 路径
    list(APPEND CMAKE_PREFIX_PATH 
        "E:/LIBS/clang+llvm-18.1.8-x86_64-pc-windows-msvc/lib/cmake/llvm"
        "E:/LIBS/clang+llvm-18.1.8-x86_64-pc-windows-msvc/lib/cmake/clang"
    )
    #link_directories(D:/visualStudio/DIA SDK/lib/amd64)
elseif(UNIX)
   
endif()


# 查找 LLVM
find_package(LLVM REQUIRED CONFIG)

# 查找 Clang
find_package(Clang REQUIRED CONFIG)

# 获取 LLVM 组件库列表
#llvm_map_components_to_libnames(LLVM_LIBS clangFormat)

# 根目录源文件
set(ROOT_SOURCES
    MainClass.h
    main.cpp
    CPHe.rc
)

# 创建主可执行目标
qt_add_executable(${PROJECT_NAME} ${ROOT_SOURCES})

# 添加子目录
add_subdirectory(gui)
add_subdirectory(code)
add_subdirectory(GUIInterface)
# 目标属性
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE FALSE
)

# 包含目录（LLVM 和 Clang 的头文件）
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${LLVM_LIBS}          # 例如 clangFormat
        libclang
        clangFormat
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
)