cmake_minimum_required(VERSION 3.9.6)

add_library(tinyerode_stb
  stb_image.h
  stb_image.c
  stb_image_write.h
  stb_image_write.c)

target_include_directories(tinyerode_stb PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(TinyErode::STB ALIAS tinyerode_stb)
