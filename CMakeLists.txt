cmake_minimum_required( VERSION 3.0 )
project(ThreadPoolLibrary)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-std=gnu++20 -fmodules-ts")

add_subdirectory( src )
