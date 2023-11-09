include(FetchContent)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 8.1.1
)

FetchContent_MakeAvailable(fmt)

# HACK(hrzgnm): disable analyzer checks on fmt library
set_target_properties(fmt PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_CPPCHECK "")

