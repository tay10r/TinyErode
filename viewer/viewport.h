#pragma once

#include <memory>

class Viewport
{
public:
  static auto Create() -> std::unique_ptr<Viewport>;

  virtual ~Viewport() = default;

  [[nodiscard]] virtual auto Setup() -> bool = 0;

  virtual void Teardown() = 0;

  virtual void Step() = 0;
};
