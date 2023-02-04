cmake_minimum_required(VERSION 3.14.7)

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
endif(NOT glm_FOUND)

add_subdirectory(imgui)
add_subdirectory(glad)
add_subdirectory(stb)

include(FetchContent)
FetchContent_GetProperties(imgui)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h"
	COMMAND $<TARGET_FILE:tinyerode_bin2c> "${imgui_SOURCE_DIR}/misc/fonts/DroidSans.ttf" "DroidSans" >"${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h")

add_executable(tinyerode_viewer WIN32
  main.cpp
  Terrain.h
  Terrain.cpp
  ErosionFilter.h
  ErosionFilter.cpp
  NoiseFilter.h
  NoiseFilter.cpp
  Renderer.h
  Renderer.cpp
  OpenGLTexture2D.h
  OpenGLTexture2D.cpp
  OpenGLShader.h
  OpenGLShaderProgram.h
  OpenGLShaderProgram.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/DroidSans.h")

target_include_directories(tinyerode_viewer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_definitions(tinyerode_viewer PRIVATE GLFW_INCLUDE_NONE=1)

target_link_libraries(tinyerode_viewer
  PRIVATE
    TinyErode::TinyErode
    glfw
    glm
    TinyErode::ImGui
    TinyErode::Glad
    TinyErode::STB)

set_target_properties(tinyerode_viewer
  PROPERTIES
    OUTPUT_NAME view
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if(UNIX)
  target_link_libraries(tinyerode_viewer PRIVATE dl)
endif(UNIX)
