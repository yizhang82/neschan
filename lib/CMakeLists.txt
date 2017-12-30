include_directories("$(PROJECT_SOURCE_DIR)/inc")
include_directories("$(PROJECT_SOURCE_DIR)")
project(NESCHANLIB C CXX)
set(CMAKE_CXX_STANDARD 14) 


file(GLOB_RECURSE NESCHANLIB_SOURCES "./src/*.cpp")

add_library(NESCHANLIB ${NESCHANLIB_SOURCES})

