add_library(doctest_dll dll.cpp)
add_library(doctest::dll ALIAS doctest_dll)
target_link_libraries(doctest_dll PRIVATE doctest::doctest)

add_executable(doctest_exe main.cpp)
target_precompile_headers(doctest_exe REUSE_FROM monkey_lib)
target_link_libraries(doctest_exe PRIVATE doctest::doctest doctest::dll
                                          fmt::fmt monkey::lib)
add_executable(doctest::exe ALIAS doctest_exe)

set_property(TARGET doctest_exe PROPERTY OUTPUT_NAME doctest)
target_compile_features(doctest_exe PRIVATE cxx_std_20)

# doctest_discover_tests(doctest_exe)
