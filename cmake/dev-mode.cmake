include(cmake/folders.cmake)

include(CTest)
add_subdirectory(test)

add_custom_target(
  run-exe
  COMMENT runs
  executable
  COMMAND cappuchin_exe
  VERBATIM)
add_dependencies(run-exe cappuchin_exe)

include(cmake/lint-targets.cmake)
include(cmake/spell-targets.cmake)

add_folders(Project)
