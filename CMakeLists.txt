cmake_minimum_required (VERSION 2.6)
project (NESCHAN CXX)
set(CMAKE_CXX_STANDARD 14)

# Locate SDL include/lib 
if(APPLE)
   #SET(GUI_TYPE MACOSX_BUNDLE)
   find_path(SDL2_INCLUDE_DIR SDL.h)
   find_library(SDL2_LIBRARY SDL2)
   mark_as_advanced(SDL2_LIBRARY)
   SET(EXTRA_LIBS ${SDL2_LIBRARY})
endif(APPLE)

 # Include directories
include_directories("$(PROJECT_SOURCE_DIR)/lib/inc" ${SDL2_INCLUDE_DIR})

add_subdirectory(lib)
add_subdirectory(test)
add_executable(NESCHAN_APP src/neschan.cpp)
set_target_properties(NESCHAN_APP PROPERTIES OUTPUT_NAME "neschan")
target_link_libraries(NESCHAN_APP NESCHANLIB ${SDL2_LIBRARY})
