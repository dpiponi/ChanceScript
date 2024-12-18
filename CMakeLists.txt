# CMake minimum version requirement
cmake_minimum_required(VERSION 3.10)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Project name and version
project(ChanceScript VERSION 1.0)

add_library(ChanceScript INTERFACE)

# Add the executable (replace main.cpp with your source files)
add_executable(ex1 src/ex1.cpp)
add_executable(ex2 src/ex2.cpp)

# Include directories (optional, if you have headers in include/)
target_include_directories(ChanceScript INTERFACE include)

target_link_libraries(ex1 PRIVATE ChanceScript)
target_link_libraries(ex2 PRIVATE ChanceScript)
