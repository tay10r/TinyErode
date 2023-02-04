cmake_minimum_required(VERSION 3.14.7)

include(FetchContent)
FetchContent_Declare(imgui URL "https://github.com/ocornut/imgui/archive/master.zip")
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

if(MSVC)
  target_compile_definitions(tinyerode_imgui PRIVATE -D_USE_MATH_DEFINES=1)
endif(MSVC)

target_include_directories(tinyerode_imgui
  PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends")

target_link_libraries(tinyerode_imgui PRIVATE glfw)

add_library(TinyErode::ImGui ALIAS tinyerode_imgui)
