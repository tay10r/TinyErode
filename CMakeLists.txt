cmake_minimum_required(VERSION 3.9.6)

project(TinyErode CXX)

option(TINYERODE_TEST "Whether or not to build the test program." OFF)
option(TINYERODE_OPENMP "Whether or not to use OpenMP." ON)

if(ERODE_OPENMP)
  find_package(OpenMP)
endif(ERODE_OPENMP)

add_library(tiny_erode INTERFACE)

target_include_directories(tiny_erode INTERFACE "${PROJECT_SOURCE_DIR}")

if(TINYERODE_OPENMP AND OpenMP_FOUND)
  target_link_libraries(tiny_erode INTERFACE OpenMP::OpenMP_CXX)
endif(TINYERODE_OPENMP AND OpenMP_FOUND)

add_library(TinyErode::TinyErode ALIAS tiny_erode)

if(TINYERODE_TEST)
  add_subdirectory(tests)
endif(TINYERODE_TEST)
