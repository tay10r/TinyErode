cmake_minimum_required(VERSION 3.9.6)

add_library(erode_test_base OBJECT
  stb_impl.cpp
  debug.hpp
  debug.cpp)

function(add_erode_test name)

  set(target erode_test_${name})

  add_executable(${target} ${ARGN} $<TARGET_OBJECTS:erode_test_base>)

  target_link_libraries(${target} PRIVATE Erode::Erode)

  if(NOT MSVC)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror -Wfatal-errors)
  endif(NOT MSVC)

endfunction(add_erode_test name)

add_erode_test(waterflow waterflow.cpp)
