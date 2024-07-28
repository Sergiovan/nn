cmake_minimum_required(VERSION 3.29)

list(APPEND CORE_SOURCES
  "util/types.cpp"
)

add_library(core STATIC)

target_sources(core
  PUBLIC
  FILE_SET CXX_MODULES FILES
  ${CORE_SOURCES}
)

target_link_libraries(core)