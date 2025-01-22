#pragma once

#include "landbrush.h"

namespace landbrush {

/// @brief A backend implementation that uses OpenGL as an accelerator.
class opengl_backend : public backend
{
public:
  using loader = auto(*)(const char* name) -> void*;

  /// @brief Creates a new instance of the OpenGL backend.
  ///
  /// @param ldr The OpenGL API function loader.
  static auto create(loader ldr) -> std::shared_ptr<opengl_backend>;

  ~opengl_backend() override = default;
};

} // namespace landbrush
