cmake_minimum_required(VERSION 3.9.6)

enable_language(C)

add_library(glad
  include/glad/glad.h
  include/KHR/khrplatform.h
  src/glad.c)

target_include_directories(glad PUBLIC include)

set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
