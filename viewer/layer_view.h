#pragma once

#include <memory>

class LayerView
{
public:
  static auto Create() -> std::unique_ptr<LayerView>;

  virtual ~LayerView() = default;

  virtual void Setup() = 0;

  virtual void Teardown() = 0;

  virtual void Step() = 0;
};
