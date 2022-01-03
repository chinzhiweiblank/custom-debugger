cmake_minimum_required (VERSION 3.0)
project (dbg)

add_compile_options(-std=c++14)

include_directories(deps/libelfin deps/linenoise include)
add_executable(dbg src/debugger.cpp src/breakpoint.cpp deps/linenoise/linenoise.c)

add_executable(test examples/test.cpp)
set_target_properties(test
                      PROPERTIES COMPILE_FLAGS "-g -O0")

add_executable(example examples/example.cpp)
set_target_properties(example
                      PROPERTIES COMPILE_FLAGS "-gdwarf-2 -O0")

add_custom_target(
   libelfin
   COMMAND make
   WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/deps/libelfin
)

target_link_libraries(dbg
                      ${PROJECT_SOURCE_DIR}/deps/libelfin/dwarf/libdwarf++.so
                      ${PROJECT_SOURCE_DIR}/deps/libelfin/elf/libelf++.so)
add_dependencies(dbg libelfin)
