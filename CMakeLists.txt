cmake_minimum_required(VERSION 3.9.6)

project(erode CXX)

option(ERODE_TESTS "Whether or not to build the tests." OFF)

find_package(OpenMP)

add_library(tiny_erode INTERFACE)

target_include_directories(tiny_erode INTERFACE "${PROJECT_SOURCE_DIR}")

if(OpenMP_FOUND)
  target_link_libraries(tiny_erode INTERFACE OpenMP::OpenMP_CXX)
endif(OpenMP_FOUND)

add_library(TinyErode::TinyErode ALIAS tiny_erode)

add_library(erode INTERFACE)

target_include_directories(erode INTERFACE "${PROJECT_SOURCE_DIR}")

add_library(Erode::Erode ALIAS erode)

if(ERODE_TESTS)
  add_subdirectory(tests)
endif(ERODE_TESTS)
