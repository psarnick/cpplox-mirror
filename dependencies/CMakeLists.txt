include(FetchContent)

#=====================================
# Helper functions and macros
#=====================================

# Adds external dependency into a project
function(add_external_dependency dependency)
  add_subdirectory(${${dependency}_SOURCE_DIR} ${${dependency}_BINARY_DIR})
endfunction()

#=====================================
# Options and initial settings
#=====================================

message(STATUS "Fetching third party libraries")
option(LINK_DEPS_STATIC CACHE ON)

set(GOOGLETEST_VERSION 1.13)

if (NOT LINK_DEPS_STATIC)
  set(BUILD_SHARED_LIBS ON)
else()
  set(BUILD_SHARED_LIBS OFF)
endif()

#=====================================
# FetchContent declarations
#=====================================

if(BUILD_TESTING)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG b796f7d44681514f58a683a3a71ff17c94edb0c1 # 1.13.0
  )
endif()

#=====================================

if(BUILD_TESTING)
  add_subdirectory(googletest)
endif()