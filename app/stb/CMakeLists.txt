cmake_minimum_required(VERSION 3.9.6)

add_library(stb
  stb_image.h
  stb_image.c
  stb_image_write.h
  stb_image_write.c)

target_include_directories(stb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(stb
  PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
