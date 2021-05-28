cmake_minimum_required(VERSION 3.9.6)

project(erode CXX)

option(ERODE_TESTS "Whether or not to build the tests." OFF)

add_library(erode INTERFACE)

target_include_directories(erode INTERFACE "${PROJECT_SOURCE_DIR}")

add_library(Erode::Erode ALIAS erode)

if(ERODE_TESTS)
  add_subdirectory(tests)
endif(ERODE_TESTS)
