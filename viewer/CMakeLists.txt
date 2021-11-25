cmake_minimum_required(VERSION 3.14.7)

find_package(glfw3 REQUIRED)

add_subdirectory(imgui)
add_subdirectory(glad)
add_subdirectory(stb)

add_executable(tinyerode_viewer
  main.cpp
  Terrain.h
  Terrain.cpp
  NoiseFilter.h
  NoiseFilter.cpp
  OpenGLTexture2D.h
  OpenGLTexture2D.cpp
  OpenGLShader.h
  OpenGLShaderProgram.h
  OpenGLShaderProgram.cpp)

target_compile_definitions(tinyerode_viewer PRIVATE GLFW_INCLUDE_NONE=1)

target_link_libraries(tinyerode_viewer
  PRIVATE
    TinyErode::TinyErode
    glfw
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
