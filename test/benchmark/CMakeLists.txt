project(Benchmark LANGUAGES CXX)

add_executable(benchmark source/main.cpp)

target_precompile_headers(benchmark REUSE_FROM monkey_lib)
target_link_libraries(benchmark PRIVATE monkey::lib fmt::fmt)
target_compile_features(benchmark PRIVATE cxx_std_20)

add_folders(Benchmark)
