cmake_minimum_required(VERSION 3.9.6)

add_executable(landbrush_example example.cpp)

set_target_properties(landbrush_example
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    OUTPUT_NAME run_example)

target_link_libraries(landbrush_example PRIVATE landbrush::landbrush)
