# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(lexereagenTests LANGUAGES CXX)

# ---- Tests ----

add_executable(lexereagen_test
    source/compiler_tests.cpp
    source/eval_tests.cpp
    source/lexing_tests.cpp
    source/parsing_tests.cpp
    source/testutils.cpp
)
target_precompile_headers(lexereagen_test REUSE_FROM lexereagen_lib)
target_link_libraries(lexereagen_test PRIVATE lexereagen_lib gtest gtest_main) 
target_compile_features(lexereagen_test PRIVATE cxx_std_20)

add_test(NAME lexereagen_test COMMAND lexereagen_test)

# ---- End-of-file commands ----

add_folders(Test)
