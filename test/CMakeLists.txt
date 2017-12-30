include_directories("$(PROJECT_SOURCE_DIR)/inc")
include_directories("$(PROJECT_SOURCE_DIR)/../lib/inc")

project(NESCHAN_TEST C CXX)
set(CMAKE_CXX_STANDARD 14) 

file(GLOB_RECURSE NESCHAN_TEST_SOURCES "./*.cpp")

add_executable(NESCHAN_TEST_EXE ${NESCHAN_TEST_SOURCES})
set_target_properties(NESCHAN_TEST_EXE PROPERTIES OUTPUT_NAME "test")
target_link_libraries(NESCHAN_TEST_EXE NESCHANLIB)

