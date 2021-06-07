cmake_minimum_required(VERSION 3.9.6)

add_library(erode_test_stb stb_impl.cpp)

add_executable(erode_test
  main.cpp
  debug.hpp
  debug.cpp)

target_link_libraries(erode_test PRIVATE TinyErode::TinyErode erode_test_stb)

if(NOT MSVC)
  target_compile_options(erode_test PRIVATE -Wall -Wextra -Werror -Wfatal-errors)
endif(NOT MSVC)

set_target_properties(erode_test
  PROPERTIES
    OUTPUT_NAME run_test
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
