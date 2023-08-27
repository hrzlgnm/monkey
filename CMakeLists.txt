cmake_minimum_required(VERSION 3.18)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(cmake/prelude.cmake)
project(
  monkey
  VERSION 0.1.0
  DESCRIPTION "Monkey programming language"
  HOMEPAGE_URL "https://github.com/hrzlgnm/monkey"
  LANGUAGES CXX)

find_package(fmt REQUIRED)

# ---- Declare library ----
add_library(
  monkey_lib OBJECT
  source/ast/array_expression.cpp
  source/ast/binary_expression.cpp
  source/ast/boolean.cpp
  source/ast/builtin_function_expression.cpp
  source/ast/callable_expression.cpp
  source/ast/call_expression.cpp
  source/ast/function_expression.cpp
  source/ast/hash_literal_expression.cpp
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
  source/eval/ast_eval.cpp
  source/eval/environment.cpp
  source/eval/object.cpp
  source/lexer/lexer.cpp
  source/lexer/token.cpp
  source/lexer/token_type.cpp
  source/parser/parser.cpp
  source/vm/vm.cpp)

add_library(monkey::lib ALIAS monkey_lib)
target_precompile_headers(
  monkey_lib
  PRIVATE
  <array>
  <algorithm>
  <cstdint>
  <deque>
  <fmt/core.h>
  <fmt/format.h>
  <iostream>
  <map>
  <memory>
  <optional>
  <string>
  <string_view>
  <variant>)

target_include_directories(
  monkey_lib PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>")
target_compile_definitions(monkey_lib PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)
target_compile_features(monkey_lib PUBLIC cxx_std_20)
target_link_libraries(monkey_lib PRIVATE fmt::fmt doctest::doctest doctest::dll)

add_executable(monkey_exe source/main.cpp)
add_executable(monkey::exe ALIAS monkey_exe)
target_compile_definitions(monkey_exe PRIVATE DOCTEST_CONFIG_DISABLE)

set_property(TARGET monkey_exe PROPERTY OUTPUT_NAME monkey)

target_compile_features(monkey_exe PRIVATE cxx_std_20)

target_link_libraries(monkey_exe PRIVATE monkey_lib)

include(cmake/dev-mode.cmake)
