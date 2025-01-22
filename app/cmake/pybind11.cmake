include_guard()

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(pybind11
  URL "https://github.com/pybind/pybind11/archive/refs/tags/v2.13.6.zip"
  URL_HASH "SHA256=d0a116e91f64a4a2d8fb7590c34242df92258a61ec644b79127951e821b47be6")
FetchContent_MakeAvailable(pybind11)
