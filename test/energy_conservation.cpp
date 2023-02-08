#include <gtest/gtest.h>

#include "TinyErode.h"

using namespace TinyErode;

namespace {

class Texture final
{
public:
  Texture(const int w, const int h) noexcept
    : m_width(w)
    , m_height(h)
    , m_data(w * h)
  {
  }

  [[nodiscard]] const float& operator()(const int x, const int y) const { return m_data[(y * m_width) + x]; }

  [[nodiscard]] float& operator()(const int x, const int y) { return m_data[(y * m_width) + x]; }

  [[nodiscard]] int width() const { return m_width; }

  [[nodiscard]] int height() const { return m_height; }

  const auto begin() const { return m_data.begin(); }

  const auto end() const { return m_data.end(); }

private:
  const int m_width;

  const int m_height;

  std::vector<float> m_data;
};

} // namespace

TEST(energy_conservation, test_1)
{
  const int w = 3;
  const int h = 3;

  Texture water(w, h);
  water(0, 0) = 1;

  Texture terrain(w, h);
  terrain(0, 0) = 0.2;
  terrain(1, 0) = 0.15;
  terrain(2, 0) = 0.14;
  terrain(0, 1) = 0.11;
  terrain(1, 1) = 0.13;
  terrain(2, 1) = 0.19;
  terrain(0, 2) = 0.20;
  terrain(1, 2) = 0.09;
  terrain(2, 2) = 0.11;

  const float totalInitialTerrain = std::accumulate(&terrain(0, 0), &terrain(2, 2), 0.0f);

  auto getHeight = [&terrain](const int x, const int y) { return terrain(x, y); };

  auto getWater = [&water](const int x, const int y) { return water(x, y); };

  auto addWater = [&water](int x, int y, float waterDelta) -> float {
    return water(x, y) = std::max(0.0f, water(x, y) + waterDelta);
  };

  auto addHeight = [&terrain](const int x, const int y, const float deltaHeight) { terrain(x, y) += deltaHeight; };

  auto carryCapacity = [](int, int) -> float { return 0.01; };

  auto deposition = [](int, int) -> float { return 0.1; };

  auto erosion = [](int, int) -> float { return 0.1; };

  auto evaporation = [](int, int) -> float { return 0.1; };

  Simulation simulation(w, h);

  simulation.SetTimeStep(1e-3);

  simulation.SetMetersPerX(1);
  simulation.SetMetersPerY(1);

  for (int i = 0; i < 4; i++) {

    simulation.ComputeFlowAndTilt(getHeight, getWater);

    simulation.TransportWater(addWater);

    simulation.TransportSediment(carryCapacity, deposition, erosion, addHeight);

    simulation.Evaporate(addWater, evaporation);
  }

  simulation.TerminateRainfall(addHeight);

  const float totalFinalTerrain = std::accumulate(&terrain(0, 0), &terrain(2, 2), 0.0f);

  EXPECT_NEAR(totalInitialTerrain, totalFinalTerrain, 0.1f);
}
