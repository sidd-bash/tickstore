cmake_minimum_required(VERSION 3.20)
project(tickstore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(tickstore src/format.cpp)
target_include_directories(tickstore PUBLIC src)

enable_testing()
add_executable(test_format tests/test_format.cpp)
target_link_libraries(test_format tickstore)
add_test(NAME format COMMAND test_format)
