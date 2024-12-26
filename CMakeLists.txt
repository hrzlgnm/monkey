cmake_minimum_required(VERSION 3.18)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(cmake/prelude.cmake)
project(
  cappuchin
  VERSION 0.1.0
  DESCRIPTION "Cappuchin programming language"
  HOMEPAGE_URL "https://github.com/hrzlgnm/Cappuchin"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/fmt.cmake)

# ---- Declare library ----
add_library(
  cappuchin_lib OBJECT
)
add_library(cappuchin::lib ALIAS cappuchin_lib)

target_sources(cappuchin_lib
  PRIVATE
    source/analyzer/analyzer.cpp
    source/analyzer/ast_analyzer.cpp
    source/ast/array_literal.cpp
    source/ast/assign_expression.cpp
    source/ast/binary_expression.cpp
    source/ast/boolean_literal.cpp
    source/ast/builtin_function.cpp
    source/ast/call_expression.cpp
    source/ast/decimal_literal.cpp
    source/ast/function_literal.cpp
    source/ast/hash_literal.cpp
    source/ast/identifier.cpp
    source/ast/if_expression.cpp
    source/ast/index_expression.cpp
    source/ast/integer_literal.cpp
    source/ast/program.cpp
    source/ast/statements.cpp
    source/ast/string_literal.cpp
    source/ast/unary_expression.cpp
    source/code/code.cpp
    source/compiler/ast_compile.cpp
    source/compiler/compiler.cpp
    source/compiler/symbol_table.cpp
    source/eval/environment.cpp
    source/eval/evaluator.cpp
    source/lexer/lexer.cpp
    source/lexer/token.cpp
    source/lexer/token_type.cpp
    source/object/object.cpp
    source/parser/parser.cpp
    source/vm/vm.cpp
)

target_precompile_headers(cappuchin_lib
  PRIVATE
    <algorithm>
    <array>
    <cstddef>
    <cstdint>
    <deque>
    <fmt/base.h>
    <fmt/format.h>
    <fmt/ostream.h>
    <fmt/ranges.h>
    <iterator>
    <map>
    <optional>
    <ostream>
    <string>
    <string_view>
    <utility>
    <unordered_map>
    <variant>
    <vector>
)

target_include_directories(
  cappuchin_lib PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>")
target_compile_definitions(cappuchin_lib PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
if(MSVC)
    target_compile_definitions(cappuchin_lib
    PUBLIC DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS)
endif()
target_compile_features(cappuchin_lib PUBLIC cxx_std_20)
target_link_libraries(cappuchin_lib PRIVATE doctest::dll doctest::doctest fmt::fmt)

add_executable(cappuchin_exe source/main.cpp)
add_executable(cappuchin::exe ALIAS cappuchin_exe)
target_compile_definitions(cappuchin_exe PRIVATE DOCTEST_CONFIG_DISABLE)
set_property(TARGET cappuchin_exe PROPERTY OUTPUT_NAME cappuchin)
target_compile_features(cappuchin_exe PRIVATE cxx_std_20)
target_link_libraries(cappuchin_exe PRIVATE fmt::fmt cappuchin_lib)

include(cmake/dev-mode.cmake)
