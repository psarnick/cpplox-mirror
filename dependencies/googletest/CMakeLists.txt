if(NOT googletest_POPULATED)
FetchContent_GetProperties(googletest)
  message(STATUS "Fetching Google Test")

  FetchContent_Populate(googletest)

  if (MSVC AND NOT USE_STATIC_RUNTIME)
    set(gtest_force_shared_crt CACHE INTERNAL ON)
  endif()

  add_external_dependency(googletest)
endif()