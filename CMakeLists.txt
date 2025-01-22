cmake_minimum_required(VERSION 3.9.6)

project(landbrush
  DESCRIPTION "A library for procedural and interactive terrain editing."
  VERSION 2.0
  LANGUAGES CXX)

option(LANDBRUSH_OPENMP  "Whether or not to use OpenMP." ON)
option(LANDBRUSH_EXAMPLE "Whether or not to build the example program." OFF)
option(LANDBRUSH_APP     "Whether or not to build the app." OFF)
option(LANDBRUSH_PYTHON  "Whether or not to build the Python library." OFF)

if(LANDBRUSH_OPENMP)
  find_package(OpenMP)
endif(LANDBRUSH_OPENMP)

add_library(landbrush
  landbrush.h
  landbrush.cpp
  landbrush_opengl.h
  landbrush_opengl_internal.h
  landbrush_opengl.cpp)

if(LANDBRUSH_PYTHON)
  set_target_properties(landbrush PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_include_directories(landbrush
  PUBLIC
    "${PROJECT_SOURCE_DIR}")

if(LANDBRUSH_OPENMP AND OpenMP_FOUND)
  target_link_libraries(landbrush INTERFACE OpenMP::OpenMP_CXX)
endif(LANDBRUSH_OPENMP AND OpenMP_FOUND)

add_library(landbrush::landbrush ALIAS landbrush)

if(LANDBRUSH_EXAMPLE)
  #add_subdirectory(example)
endif(LANDBRUSH_EXAMPLE)

if(LANDBRUSH_PYTHON OR LANDBRUSH_APP)
  add_subdirectory(app)
endif(LANDBRUSH_PYTHON OR LANDBRUSH_APP)
