# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location
project(Tests LANGUAGES CXX)

# ---- Tests ----

add_executable(monkey_test
    source/compiler_tests.cpp
    source/eval_tests.cpp
    source/lexing_tests.cpp
    source/parsing_tests.cpp
    source/testutils.cpp
)
target_precompile_headers(monkey_test REUSE_FROM monkey_lib)
target_link_libraries(monkey_test PRIVATE monkey_lib gtest gtest_main) 
target_compile_features(monkey_test PRIVATE cxx_std_20)

add_test(NAME monkey_test COMMAND monkey_test)

# ---- End-of-file commands ----

add_folders(Test)
