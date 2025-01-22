#pragma once

#include <memory>

class SimState;
struct Config;

class LayerView
{
public:
  using BrushCallback = void (*)(void*, const float x, const float y, const float r);

  static auto Create(void* brushCallbackData, BrushCallback brushCallback) -> std::unique_ptr<LayerView>;

  virtual ~LayerView() = default;

  virtual void Setup() = 0;

  virtual void Teardown() = 0;

  virtual void Step(const Config& config) = 0;

  virtual void Update(SimState& state) = 0;
};
