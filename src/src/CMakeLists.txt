add_library(cpplox)

target_include_directories(cpplox
  PUBLIC
    ${cpplox_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(Treewalk)
add_subdirectory(Bytecode)

add_executable(cpplox_repl)
set_target_properties(cpplox_repl PROPERTIES OUTPUT_NAME "cpplox")

target_include_directories(cpplox_repl
  PUBLIC
    ${cpplox_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(cpplox_repl
  PUBLIC
    main.cpp
)

target_link_libraries(cpplox_repl PRIVATE cpplox)