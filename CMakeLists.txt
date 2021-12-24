set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_DIR "${PROJECT_SOURCE_DIR}/build")
cmake_minimum_required (VERSION 3.0)
project (Dbg)

add_compile_options(-std=c++14)

include_directories(dep/libelfin deps/linenoise include)
add_executable(${BUILD_DIR}/customdbg src/debugger.cpp src/breakpoint.cpp deps/linenoise/linenoise.c)

# Examples
add_executable(${BUILD_DIR}/test examples/test.cpp)
set_target_properties(test
                      PROPERTIES COMPILE_FLAGS "-g -O0")

# Adding and making link libraries
set(PROJECT_DEPS "${PROJECT_SOURCE_DIR}/deps")
add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_DEPS}/libelfin
)

target_link_libraries(customdbg
                      ${PROJECT_DEPS}/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_DEPS}/libelfin/elf/libelf++.so)
add_dependencies(customdbg libelfin)