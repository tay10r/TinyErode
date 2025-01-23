#include "SimState.h"

#include <algorithm>
#include <vector>

SimState::SimState(landbrush::backend& backend, const Config& config)
  : m_config(config)
{
  const auto w = config.width;
  const auto h = config.height;

  height_ = backend.create_texture(w, h, landbrush::format::c1);

  m_rock = backend.create_texture(w, h, landbrush::format::c1);

  m_soil.current() = backend.create_texture(w, h, landbrush::format::c1);
  m_soil.next() = backend.create_texture(w, h, landbrush::format::c1);

  m_sediment.current() = backend.create_texture(w, h, landbrush::format::c1);
  m_sediment.next() = backend.create_texture(w, h, landbrush::format::c1);

  m_water.current() = backend.create_texture(w, h, landbrush::format::c1);
  m_water.next() = backend.create_texture(w, h, landbrush::format::c1);

  m_flux.current() = backend.create_texture(w, h, landbrush::format::c4);
  m_flux.next() = backend.create_texture(w, h, landbrush::format::c4);

  {
    std::vector<float> soil_data(w * h, config.initial_soil_height);
    m_soil.current()->write(soil_data.data());
  }
}

void
SimState::HeightData(const uint8_t* data)
{
  const auto size = m_rock->get_size();

  std::vector<float> tmp(size.width * size.height, 0.0F);

  const int numCells = size.width * size.height;

  for (int i = 0; i < numCells; i++) {
    tmp[i] = (data[i] / 255.0F) * m_config.rock_height;
  }

  m_rock->write(tmp.data());
}

void
SimState::HeightData(const uint16_t* data)
{
  const auto size = m_rock->get_size();

  std::vector<float> tmp(size.total(), 0.0F);

  const int numCells = size.total();

  for (int i = 0; i < numCells; i++) {
    tmp[i] = (data[i] / 65535.0F) * m_config.rock_height;
  }

  m_rock->write(tmp.data());
}

void
SimState::HeightNoise(FastNoiseLite& noise, const float distancePerCell)
{
  const auto size = m_rock->get_size();

  std::vector<float> data(size.width * size.height, 0.0F);

  const int numCells = size.width * size.height;

  const auto x_scale = 1.0F / static_cast<float>(size.width);
  const auto y_scale = 1.0F / static_cast<float>(size.height);

  const auto aspect = static_cast<float>(size.width) / static_cast<float>(size.height);

  for (int i = 0; i < numCells; i++) {

    const auto u = x_scale * static_cast<float>(i % size.width);
    const auto v = y_scale * static_cast<float>(static_cast<int>(i / size.height));

    data[i] = std::max(noise.GetNoise(u * aspect * distancePerCell, v * distancePerCell), 0.0F) * m_config.rock_height;
  }

  m_rock->write(data.data());
}

auto
SimState::GetRockTexture() -> landbrush::texture&
{
  return *m_rock;
}

auto
SimState::GetWaterTexture() -> landbrush::texture&
{
  return *m_water.current();
}

auto
SimState::GetHeightTexture() -> landbrush::texture&
{
  return *height_;
}

void
SimState::Step(landbrush::backend& backend)
{
  for (auto i = 0; i < m_config.speedup; i++) {

    auto* fluxShader = backend.get_shader("flux");

    fluxShader->set_float("gravity", 9.8F);
    fluxShader->set_float("pipe_radius", m_config.pipeRadius);
    fluxShader->set_float("pipe_length", m_config.distancePerCell);
    fluxShader->set_float("dt", m_config.timeDelta);
    fluxShader->set_texture("rock", m_rock.get());
    fluxShader->set_texture("soil", m_soil.current().get());
    fluxShader->set_texture("water", m_water.current().get());
    fluxShader->set_texture("last_flux", m_flux.current().get());
    fluxShader->set_texture("next_flux", m_flux.next().get());
    fluxShader->invoke();

    m_flux.step();

    auto* flowShader = backend.get_shader("flow");
    flowShader->set_float("dt", m_config.timeDelta);
    flowShader->set_float("pipe_length", m_config.distancePerCell);
    flowShader->set_texture("flux", m_flux.current().get());
    flowShader->set_texture("last_water", m_water.current().get());
    flowShader->set_texture("next_water", m_water.next().get());
    flowShader->invoke();

    auto* hydroErosionShader = backend.get_shader("hydraulic_erosion");
    hydroErosionShader->set_float("carry_capacity", m_config.kc);
    hydroErosionShader->set_float("deposition", m_config.kd);
    hydroErosionShader->set_float("erosion", m_config.ke);
    hydroErosionShader->set_float("dt", m_config.timeDelta);
    hydroErosionShader->set_float("min_tilt", m_config.minTilt);
    hydroErosionShader->set_texture("flux", m_flux.current().get());
    hydroErosionShader->set_texture("rock", m_rock.get());
    hydroErosionShader->set_texture("last_soil", m_soil.current().get());
    hydroErosionShader->set_texture("next_soil", m_soil.next().get());
    hydroErosionShader->set_texture("last_water", m_water.current().get());
    hydroErosionShader->set_texture("next_water", m_water.next().get());
    hydroErosionShader->set_texture("last_sediment", m_sediment.current().get());
    hydroErosionShader->set_texture("next_sediment", m_sediment.next().get());
    hydroErosionShader->invoke();

    m_sediment.step();

    auto* hydroTransportShader = backend.get_shader("hydraulic_transport");
    hydroTransportShader->set_float("dt", m_config.timeDelta);
    hydroTransportShader->set_float("pipe_length", m_config.distancePerCell);
    hydroTransportShader->set_texture("flux", m_flux.current().get());
    hydroTransportShader->set_texture("last_water", m_water.current().get());
    hydroTransportShader->set_texture("next_water", m_water.next().get());
    hydroTransportShader->set_texture("last_sediment", m_sediment.current().get());
    hydroTransportShader->set_texture("next_sediment", m_sediment.next().get());
    hydroTransportShader->invoke();

    m_soil.step();

    auto* blendShader = backend.get_shader("blend");
    blendShader->set_float("k_alpha", 1.0F);
    blendShader->set_float("k_beta", 1.0F);
    blendShader->set_texture("alpha", m_rock.get());
    blendShader->set_texture("beta", m_soil.current().get());
    blendShader->set_texture("gamma", height_.get());
    blendShader->invoke();

    m_sediment.step();

    m_water.step();
  }
}

auto
SimState::GetSoilTexture() -> Texture&
{
  return *m_soil.current();
}

auto
SimState::GetSedimentTexture() -> Texture&
{
  return *m_sediment.current();
}
