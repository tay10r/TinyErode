cmake_minimum_required(VERSION 3.9.6)

project(TinyErode CXX)

option(TINYERODE_TEST "Whether or not to build the test program." OFF)
option(TINYERODE_OPENMP "Whether or not to use OpenMP." ON)
option(TINYERODE_EXAMPLE "Whether or not to build the example program." OFF)

if(TINYERODE_OPENMP)
  find_package(OpenMP)
endif(TINYERODE_OPENMP)

add_library(tinyerode INTERFACE)

target_include_directories(tinyerode INTERFACE "${PROJECT_SOURCE_DIR}")

if(TINYERODE_OPENMP AND OpenMP_FOUND)
  target_link_libraries(tinyerode INTERFACE OpenMP::OpenMP_CXX)
endif(TINYERODE_OPENMP AND OpenMP_FOUND)

add_library(TinyErode::TinyErode ALIAS tinyerode)

if(TINYERODE_EXAMPLE)
  add_subdirectory(example)
endif(TINYERODE_EXAMPLE)

if(TINYERODE_TEST)
  add_subdirectory(test)
endif(TINYERODE_TEST)
