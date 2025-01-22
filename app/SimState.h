#pragma once

#include <landbrush.h>

#include "Config.h"

#include "FastNoiseLite.h"

class SimState final
{
public:
  template<typename T>
  using SwapBuffer = landbrush::swap_buffer<T>;

  using Texture = landbrush::texture;

  SimState(landbrush::backend& backend, const Config& config);

  void HeightData(const uint8_t* data);

  void HeightData(const uint16_t* data);

  void HeightNoise(FastNoiseLite& noise, const float distancePerCell);

  void Step(landbrush::backend& backend);

  auto GetRockTexture() -> Texture&;

  auto GetWaterTexture() -> Texture&;

  auto GetWaterTextureSB() -> SwapBuffer<std::shared_ptr<Texture>>& { return m_water; }

  auto GetConfig() const -> const Config& { return m_config; }

private:
  std::shared_ptr<Texture> m_rock;

  SwapBuffer<std::shared_ptr<Texture>> m_water;

  SwapBuffer<std::shared_ptr<Texture>> m_flux;

  SwapBuffer<std::shared_ptr<Texture>> m_soil;

  SwapBuffer<std::shared_ptr<Texture>> m_sediment;

  Config m_config;
};
