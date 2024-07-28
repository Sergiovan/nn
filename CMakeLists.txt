cmake_minimum_required(VERSION 3.29)

project(nn
  VERSION 0.0.1
  DESCRIPTION "Toy language for now"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

add_executable(nn src/main.cpp)

set_target_properties(nn PROPERTIES RUNTIME_OUTPUT_DIRECTORY "target/")

target_link_libraries(nn core)