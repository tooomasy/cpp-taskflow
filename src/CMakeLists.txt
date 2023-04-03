set(SOURCES
  main.cc
)

add_executable(cpp-taskflow ${SOURCES})

target_include_directories(cpp-taskflow
  PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(cpp-taskflow pthread)