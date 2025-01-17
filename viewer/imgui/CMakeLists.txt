cmake_minimum_required(VERSION 3.14.7)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(imgui URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.91.4-docking.zip")
FetchContent_MakeAvailable(imgui)
FetchContent_GetProperties(imgui)

add_executable(tinyerode_bin2c
  "${imgui_SOURCE_DIR}/misc/fonts/binary_to_compressed_c.cpp")

add_library(tinyerode_imgui
  "${imgui_SOURCE_DIR}/imgui.h"
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp")

target_compile_definitions(tinyerode_imgui PUBLIC IMGUI_IMPL_OPENGL_ES3=1 GLFW_INCLUDE_NONE=1)

if(MSVC)
  target_compile_definitions(tinyerode_imgui PRIVATE _USE_MATH_DEFINES=1)
endif(MSVC)

target_include_directories(tinyerode_imgui
  PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends")

target_link_libraries(tinyerode_imgui PRIVATE glfw tinyerode_glad)

add_library(TinyErode::ImGui ALIAS tinyerode_imgui)
