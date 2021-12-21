cmake_minimum_required (VERSION 3.0)
project (Dbg)

add_compile_options(-std=c++14)
find_package(linenoise)
add_executable(dbg src/dbg.cpp )
