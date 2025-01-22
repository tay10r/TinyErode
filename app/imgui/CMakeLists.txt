cmake_minimum_required(VERSION 3.14.7)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(imgui URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.91.4-docking.zip")
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui)

add_executable(imgui_bin2c
  "${imgui_SOURCE_DIR}/misc/fonts/binary_to_compressed_c.cpp")

add_library(imgui
  "${imgui_SOURCE_DIR}/imgui.h"
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp")

set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3=1 GLFW_INCLUDE_NONE=1)

if(MSVC)
  target_compile_definitions(imgui PRIVATE _USE_MATH_DEFINES=1)
endif(MSVC)

target_include_directories(imgui
  PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/misc/cpp"
    "${imgui_SOURCE_DIR}/backends")

target_link_libraries(imgui PRIVATE glfw glad)
