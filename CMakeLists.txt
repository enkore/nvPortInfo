cmake_minimum_required(VERSION 3.28)
project(nvportinfo)

set(CMAKE_CXX_STANDARD 17)

include_directories(nvapi)
link_directories(nvapi/amd64/)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_executable(nvPortInfo main.cpp)
target_link_libraries(nvPortInfo nvapi64.lib)
