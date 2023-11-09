include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1
)

FetchContent_MakeAvailable(fmt)

# HACK(hrzgnm): disable analyzer checks on fmt library
set_target_properties(fmt PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_CPPCHECK "")

