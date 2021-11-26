#include "ErosionFilter.h"

#include "Terrain.h"

#include <TinyErode.h>

#include <imgui.h>

#include <memory>

#include <cstring>

namespace {

struct ErodeState final
{
  int w = 0;
  int h = 0;

  TinyErode::Simulation simulation;

  std::vector<float> heightMap;

  std::vector<float> water;

  ErodeState(const Terrain& terrain)
    : w(terrain.width())
    , h(terrain.height())
    , simulation(w, h)
    , heightMap(w * h)
    , water(w * h, 0.1f)
  {
    simulation.SetMetersPerX(terrain.totalMetersX() / w);
    simulation.SetMetersPerY(terrain.totalMetersY() / h);

    std::memcpy(&heightMap[0], terrain.getHeightMapBuffer(), w * h * sizeof(float));
  }

  void erode();
};

} // namespace

class ErosionFilterImpl final
{
  friend ErosionFilter;

  bool preview = true;

  std::unique_ptr<ErodeState> erodeState;
};

ErosionFilter::~ErosionFilter()
{
  delete m_self;
}

ErosionFilterImpl&
ErosionFilter::getSelf()
{
  if (!m_self)
    m_self = new ErosionFilterImpl();

  return *m_self;
}

void
ErosionFilter::renderGui(const Terrain& terrain)
{
  ErosionFilterImpl& self = getSelf();

  ImGui::Checkbox("Preview", &self.preview);

  if (ImGui::Button("Apply"))
    getSelf().erodeState.reset(new ErodeState(terrain));
}

bool
ErosionFilter::inErodeState()
{
  if (!m_self)
    return false;

  return m_self->erodeState.get() != nullptr;
}

void
ErosionFilter::erode(Terrain& terrain)
{
  if (!m_self || (m_self->erodeState.get() == nullptr))
    return;

  m_self->erodeState->erode();

  const int w = m_self->erodeState->w;
  const int h = m_self->erodeState->h;

  terrain.setHeightMap(m_self->erodeState->heightMap.data(), w, h);

  terrain.setWaterMap(m_self->erodeState->water.data(), w, h);
}

namespace {

void
ErodeState::erode()
{
  auto getHeight = [this](int x, int y) { return heightMap[(y * w) + x]; };

  auto getWater = [this](int x, int y) { return water[(y * w) + x]; };

  auto addWater = [this](int x, int y, float waterDelta) -> float {
    return water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + waterDelta);
  };

  auto addHeight = [this](int x, int y, float deltaHeight) { heightMap[(y * w) + x] += deltaHeight; };

  auto carryCapacity = [](int, int) -> float { return 0.01; };

  auto deposition = [](int, int) -> float { return 0.1; };

  auto erosion = [](int, int) -> float { return 0.1; };

  auto evaporation = [](int, int) -> float { return 0.00001; };

  simulation.ComputeFlowAndTilt(getHeight, getWater);

  simulation.TransportWater(addWater);

  simulation.TransportSediment(carryCapacity, deposition, erosion, addHeight);

  simulation.Evaporate(addWater, evaporation);

  // simulation.TerminateRainfall(addHeight);
}

} // namespace
