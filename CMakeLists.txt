cmake_minimum_required(VERSION 3.30)

project(nn
  VERSION 0.0.1
  DESCRIPTION "Toy language for now"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fdiagnostics-color=always")

add_subdirectory(src)
add_executable(nn src/main.cpp)

target_include_directories(nn
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set_target_properties(nn PROPERTIES RUNTIME_OUTPUT_DIRECTORY "target/")

target_link_libraries(nn core)