cmake_minimum_required(VERSION 3.29)

list(APPEND CORE_HEADERS
  "common/token.hpp"

  "frontend/lexer.hpp"

  "util/assert.hpp"
  "util/types.hpp"
)

list(APPEND CORE_SOURCES
  "frontend/lexer.cpp"
)

add_library(core STATIC)

target_compile_options(core
  PUBLIC
  -Wall
  -Wextra
  -Wconversion
  -Wno-sign-conversion
  -Wdouble-promotion
  -Wno-unused-parameter
  -Wno-unused-function
)

target_include_directories(core
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_precompile_headers(core
  PUBLIC
  ${CORE_HEADERS}
  "<string>"
  "<vector>"
)

target_sources(core
  PUBLIC
  ${CORE_SOURCES}
)

target_link_libraries(core PUBLIC -lstdc++exp)