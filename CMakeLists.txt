# CMake minimum version requirement
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(ChanceScript VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=1")

# Add the header-only library (include directory only)
add_library(ChanceScript INTERFACE)

# Specify the include directories for the library
target_include_directories(ChanceScript INTERFACE include)

# Add example executables
add_executable(ex1 src/ex1.cpp)
add_executable(ex2 src/ex2.cpp)

# Link the header-only library to the example executables
target_link_libraries(ex1 PRIVATE ChanceScript)
target_link_libraries(ex2 PRIVATE ChanceScript)
