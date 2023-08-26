include(cmake/folders.cmake)

include(CTest)
add_subdirectory(test)

add_custom_target(
  run-exe
  COMMENT runs
  executable
  COMMAND monkey_exe
  VERBATIM)
add_dependencies(run-exe monkey_exe)

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
