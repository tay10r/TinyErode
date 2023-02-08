cmake_minimum_required(VERSION 3.9.6)

include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
FetchContent_Declare(googletest URL "https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip")
FetchContent_MakeAvailable(googletest)

add_executable(tinyerode_tests
  energy_conservation.cpp)

target_link_libraries(tinyerode_tests PRIVATE TinyErode::TinyErode gtest gtest_main)

if(NOT MSVC)
  target_compile_options(tinyerode_tests PRIVATE -Wall -Wextra -Werror -Wfatal-errors)
endif(NOT MSVC)

set_target_properties(tinyerode_tests
  PROPERTIES
    OUTPUT_NAME run_tests
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
