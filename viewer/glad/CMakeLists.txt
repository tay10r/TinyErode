cmake_minimum_required(VERSION 3.9.6)

enable_language(C)

add_library(tinyerode_glad
  include/glad/glad.h
  include/KHR/khrplatform.h
  src/glad.c)

target_include_directories(tinyerode_glad PUBLIC include)

add_library(TinyErode::Glad ALIAS tinyerode_glad)
