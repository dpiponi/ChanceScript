cmake_minimum_required(VERSION 3.10)

project(ChanceScript VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(googletest)
enable_testing() 

add_executable(ChanceScriptTests tests/test_main.cpp) 
target_link_libraries(ChanceScriptTests PRIVATE gtest_main ChanceScript)
add_test(NAME ChanceScriptTests COMMAND ChanceScriptTests) 

add_library(ChanceScript INTERFACE)

target_include_directories(ChanceScript INTERFACE include)

add_executable(ex1 src/ex1.cpp)
add_executable(ex2 src/ex2.cpp)
add_executable(ex3 src/ex3.cpp)
add_executable(ex4 src/ex4.cpp)
add_executable(interpret src/interpret.cpp)

target_link_libraries(ex1 PRIVATE ChanceScript)
target_link_libraries(ex2 PRIVATE ChanceScript)
target_link_libraries(ex3 PRIVATE ChanceScript)
target_link_libraries(interpret PRIVATE ChanceScript)
