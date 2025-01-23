cmake_minimum_required(VERSION 3.14.7)

enable_language(C)

find_package(glfw3 QUIET)

if(NOT glfw3_FOUND)
  include(FetchContent)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw3 URL "https://github.com/glfw/glfw/archive/refs/tags/3.3.8.zip")
  FetchContent_MakeAvailable(glfw3)
endif(NOT glfw3_FOUND)

find_package(glm QUIET)

if(NOT glm_FOUND)
  include(FetchContent)
  FetchContent_Declare(glm URL "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip")
  FetchContent_MakeAvailable(glm)
  if(NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
  endif(NOT TARGET glm::glm)
endif(NOT glm_FOUND)

add_subdirectory(glad)
add_subdirectory(imgui)
add_subdirectory(stb)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(cmake/pybind11.cmake)
include(cmake/implot.cmake)

include(FetchContent)
FetchContent_GetProperties(imgui)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h"
  COMMAND $<TARGET_FILE:imgui_bin2c> "${imgui_SOURCE_DIR}/misc/fonts/DroidSans.ttf" "DroidSans" >"${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h")

add_library(landbrush_app
  run.h
  run.cpp
  App.h
  App.cpp
  Viewport.h
  Viewport.cpp
  LayerView.h
  LayerView.cpp
  SimState.h
  SimState.cpp
  Config.h
  ControlWidget.h
  ControlWidget.cpp
  Terrain.h
  Terrain.cpp
  Renderer.h
  Renderer.cpp
  OpenGLTexture2D.h
  OpenGLTexture2D.cpp
  OpenGLShader.h
  OpenGLShaderProgram.h
  OpenGLShaderProgram.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h")
target_include_directories(landbrush_app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(landbrush_app PRIVATE GLFW_INCLUDE_NONE=1)
target_link_libraries(landbrush_app
  PRIVATE
    landbrush::landbrush
    glfw
    glad
    glm::glm
    stb
    imgui
    implot)
if(UNIX)
  target_link_libraries(landbrush_app PRIVATE dl)
endif(UNIX)

pybind11_add_module(pylandbrush python.cpp)
target_link_libraries(pylandbrush PRIVATE landbrush_app)

add_executable(landbrush_app_exe main.cpp)
target_link_libraries(landbrush_app_exe PRIVATE landbrush_app)
set_target_properties(landbrush
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    OUTPUT_NAME landbrush)
