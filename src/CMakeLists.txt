cmake_minimum_required(VERSION 3.29)

list(APPEND CORE_SOURCES
  "backend/output_gnu_as.cpp"

  "common/ast.cpp"
  "common/asm_ast.cpp"
  "common/error.cpp"
  "common/source.cpp"
  "common/tac.cpp"
  "common/token.cpp"

  "driver/driver.cpp"

  "frontend/lexer.cpp"
  "frontend/parser.cpp"

  "transform/asm_parser.cpp"
  "transform/tac_parser.cpp"

  "util/string.cpp"
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

  "<array>"
  "<print>"
  "<string>"
  "<sstream>"
  "<type_traits>"
  "<unordered_map>"
  "<utility>"
  "<vector>"
)

target_sources(core
  PUBLIC
  ${CORE_SOURCES}
)

target_link_libraries(core PUBLIC -lstdc++exp)
# target_compile_options(core PUBLIC -fsanitize=undefined,address)
# target_link_options(core PUBLIC -fsanitize=undefined,address)