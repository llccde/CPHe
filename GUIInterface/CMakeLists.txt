target_include_directories(${PROJECT_NAME} PRIVATE 
../code/
)
target_sources(${PROJECT_NAME} PRIVATE
NameViewTreeModel.h
NameViewTreeModel.cpp
 "SignalUniquePtr.h" "SignalUniquePtr.cpp")