set(TARGET_NAME doctest)

add_library(${TARGET_NAME} INTERFACE)
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${TARGET_NAME} INTERFACE cxx_std_20)

add_library(doctest_impl impl.cpp)
add_library(doctest::impl ALIAS doctest_impl)
target_link_libraries(doctest_impl INTERFACE doctest::doctest)

add_executable(doctest_exe main.cpp)
target_link_libraries(doctest_exe doctest::impl monkey::lib)
add_executable(doctest::exe ALIAS doctest_exe)

set_property(TARGET doctest_exe PROPERTY OUTPUT_NAME doctest)

target_compile_features(doctest_exe PRIVATE cxx_std_20)
