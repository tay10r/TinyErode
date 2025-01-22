cmake_minimum_required(VERSION 3.9.6)

add_executable(TinyErodeExample main.cpp)

set_target_properties(TinyErodeExample
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    OUTPUT_NAME run_example)

target_link_libraries(TinyErodeExample PRIVATE TinyErode::TinyErode)
