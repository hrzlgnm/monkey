project(Tests LANGUAGES CXX)

# ---- Tests ----

add_executable(
  monkey_test
  source/code_tests.cpp
  source/compiler_tests.cpp
  source/eval_tests.cpp
  source/lexing_tests.cpp
  source/parsing_tests.cpp
  source/testutils.cpp
  source/vm_tests.cpp
  source/symbol_table_tests.cpp)

target_precompile_headers(monkey_test REUSE_FROM monkey_lib)
target_link_libraries(monkey_test PRIVATE monkey::lib fmt::fmt GTest::gtest
                                          GTest::gtest_main)
target_compile_features(monkey_test PRIVATE cxx_std_20)

add_test(NAME monkey_test COMMAND monkey_test)

add_subdirectory(benchmark)
add_subdirectory(doctest)

# ---- End-of-file commands ----

add_folders(Test)
