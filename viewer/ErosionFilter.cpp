#include "ErosionFilter.h"

#include "Terrain.h"

#include <TinyErode.h>

#include "FastNoiseLite.h"

#include <imgui.h>

#include <numeric>
#include <memory>

#include <cstring>

namespace {

struct ErosionSettings final
{
  float timeStep{ 0.001 };

  float carryCapacity{ 0.02 };

  float deposition{ 0.01 };

  float erosion{ 0.01 };

  float evaporation{ 0.1 };
};

struct ErodeState final
{
  const ErosionSettings* settings;

  int w = 0;
  int h = 0;

  TinyErode::Simulation simulation;

  FastNoiseLite rainNoise;

  float rainX{ 0.5f };
  float rainY{ 0.0f };

  float windX{ 0.0f };
  float windY{ 1.0f };

  std::vector<float> heightMap;

  std::vector<float> water;

  ErodeState(const Terrain& terrain, const ErosionSettings* settingsParam)
    : settings(settingsParam),
      w(terrain.width())
      , h(terrain.height())
      , simulation(w, h)
      , heightMap(w * h)
      , water(w * h, 0.0f)
  {
    simulation.SetMetersPerX(terrain.totalMetersX() / w);
    simulation.SetMetersPerY(terrain.totalMetersY() / h);

    rainNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

    std::memcpy(&heightMap[0], terrain.getSoilBuffer(), w * h * sizeof(float));
  }

  void erode();

  float getTotalWater() const
  {
    return std::accumulate(water.begin(), water.end(), 0.0f);
  }

  void terminate();
};

} // namespace

class ErosionFilterImpl final
{
  friend ErosionFilter;

  ErosionSettings settings;

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

  const float totalWater = self.erodeState ? self.erodeState->getTotalWater() : 0.0f;

  if ((totalWater == 0.0f) && self.erodeState) {
    self.erodeState->terminate();
    self.erodeState.reset();
  }

  ImGui::SliderFloat("Time Step", &self.settings.timeStep, 0, 0.1);
  ImGui::SliderFloat("Carry Capacity", &self.settings.carryCapacity, 0, 1);
  ImGui::SliderFloat("Deposition", &self.settings.deposition, 0, 1);
  ImGui::SliderFloat("Erosion", &self.settings.erosion, 0, 1);
  ImGui::SliderFloat("Evaporation", &self.settings.evaporation, 0, 1);

  ImGui::Checkbox("Preview", &self.preview);

  ImGui::Text("Water: %f", static_cast<double>(totalWater));

  ImGui::BeginDisabled(!!self.erodeState);

  if (ImGui::Button("Apply"))
    self.erodeState.reset(new ErodeState(terrain, &self.settings));

  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(!self.erodeState);

  if (ImGui::Button("Cancel")) {
    self.erodeState->terminate();
    self.erodeState.reset();
  }

  ImGui::EndDisabled();
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

  terrain.setSoilHeight(m_self->erodeState->heightMap.data(), w, h);

  terrain.setWaterHeight(m_self->erodeState->water.data(), w, h);
}

namespace {

void
ErodeState::erode()
{
  rainNoise.SetFrequency(1.0f);

  for (int y = 0; y < h; y++) {

    for (int x = 0; x < w; x++) {

      const float u = (static_cast<float>(x) / w);
      const float v = (static_cast<float>(y) / h);

      const float distance = std::sqrt((rainX - u) * (rainX - u) + (rainY - v) * (rainY - v));

      const float drop = (rainNoise.GetNoise(u, v) + 1.0f) * 0.0005f * std::exp(-distance * distance * 8);

      water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + drop);
    }
  }

  rainX += windX * settings->timeStep;
  rainY += windY * settings->timeStep;

  auto getHeight = [this](int x, int y) { return heightMap[(y * w) + x]; };

  auto getWater = [this](int x, int y) { return water[(y * w) + x]; };

  auto addWater = [this](int x, int y, float waterDelta) -> float {
    return water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + waterDelta);
  };

  auto addHeight = [this](int x, int y, float deltaHeight) {
    heightMap[(y * w) + x] = std::max(0.0f, heightMap[(y * w) + x] + deltaHeight);
  };

  auto getCarryCapacity = [this](int, int) -> float { return settings->carryCapacity; };

  auto getDeposition = [this](int, int) -> float { return settings->deposition; };

  auto getErosion = [this](int, int) -> float { return settings->erosion; };

  auto getEvaporation = [this](int, int) -> float { return settings->evaporation; };

  simulation.SetTimeStep(settings->timeStep);

  simulation.ComputeFlowAndTilt(getHeight, getWater);

  simulation.TransportWater(addWater);

  simulation.TransportSediment(getCarryCapacity, getDeposition, getErosion, addHeight);

  simulation.Evaporate(addWater, getEvaporation);
}

void
ErodeState::terminate()
{
  auto addHeight = [this](const int x, const int y, const float deltaHeight) { heightMap[(y * w) + x] += deltaHeight; };

  simulation.TerminateRainfall(addHeight);
}

} // namespace
