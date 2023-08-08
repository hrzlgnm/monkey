cmake_minimum_required(VERSION 3.18)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(cmake/prelude.cmake)
project(
  monkey
  VERSION 0.1.0
  DESCRIPTION "The name is monkey"
  HOMEPAGE_URL "https://github.com/hrzlgnm/monkey"
  LANGUAGES CXX)
find_package(fmt REQUIRED)

# ---- Declare library ----
add_library(
  monkey_lib OBJECT
  source/compiler/code.cpp
  source/compiler/compiler.cpp
  source/compiler/vm.cpp
  source/lexer/lexer.cpp
  source/lexer/token.cpp
  source/lexer/token_type.cpp
  source/parser/array_expression.cpp
  source/parser/binary_expression.cpp
  source/parser/boolean.cpp
  source/parser/builtin_function_expression.cpp
  source/parser/callable_expression.cpp
  source/parser/call_expression.cpp
  source/parser/environment.cpp
  source/parser/function_expression.cpp
  source/parser/hash_literal_expression.cpp
  source/parser/identifier.cpp
  source/parser/if_expression.cpp
  source/parser/index_expression.cpp
  source/parser/integer_literal.cpp
  source/parser/object.cpp
  source/parser/parser.cpp
  source/parser/program.cpp
  source/parser/statements.cpp
  source/parser/string_literal.cpp
  source/parser/unary_expression.cpp
  source/parser/util.cpp)

target_precompile_headers(
  monkey_lib
  PRIVATE
  <algorithm>
  <iostream>
  <cstdint>
  <memory>
  <fmt/core.h>
  <string>
  <string_view>)

target_include_directories(
  monkey_lib
  PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
         "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/parser>"
         "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/compiler>"
         "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source/lexer>")
target_compile_features(monkey_lib PUBLIC cxx_std_20)
target_link_libraries(monkey_lib PUBLIC fmt)

add_executable(monkey_exe source/main.cpp)
add_executable(monkey::exe ALIAS monkey_exe)

set_property(TARGET monkey_exe PROPERTY OUTPUT_NAME monkey)

target_compile_features(monkey_exe PRIVATE cxx_std_20)

target_link_libraries(monkey_exe PRIVATE monkey_lib)

include(cmake/dev-mode.cmake)
