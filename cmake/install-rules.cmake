install(
    TARGETS lexereagen_exe
    RUNTIME COMPONENT lexereagen_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
