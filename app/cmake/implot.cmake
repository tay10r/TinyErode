include_guard()

include(FetchContent)

FetchContent_Declare(implot URL "https://github.com/epezent/implot/archive/refs/heads/master.zip")
FetchContent_MakeAvailable(implot)
FetchContent_GetProperties(implot)

add_library(implot
  "${implot_SOURCE_DIR}/implot.h"
  "${implot_SOURCE_DIR}/implot.cpp"
  "${implot_SOURCE_DIR}/implot_items.cpp")
target_include_directories(implot PUBLIC "${implot_SOURCE_DIR}")
target_link_libraries(implot PUBLIC imgui)
set_target_properties(implot PROPERTIES POSITION_INDEPENDENT_CODE ON)
