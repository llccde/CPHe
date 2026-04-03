# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\CPHe_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\CPHe_autogen.dir\\ParseCache.txt"
  "CPHe_autogen"
  )
endif()
