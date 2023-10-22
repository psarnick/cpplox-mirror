cmake_minimum_required(VERSION 3.13)

project(cpplox
  LANGUAGES CXX
  VERSION 0.1
)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wpedantic -Wall -Werror)

include(CTest)

add_subdirectory(dependencies)
add_subdirectory(src)

if(BUILD_TESTING AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure)
  add_subdirectory(test)
endif()

# Add executable target
add_executable(cpplox_binary src/main.cpp)

# Link against library target
target_link_libraries(cpplox_binary PRIVATE cpplox)


