#include <TinyErode.h>

#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#define _USE_MATH_DEFINES 1

#include <math.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

namespace {

bool
SavePNG(const char* imagePath,
        int w,
        int h,
        const std::vector<float>& heightMap,
        float maxHeight);

void
GenHeightMap(int w, int h, std::vector<float>& heightMap, float maxHeight)
{
  int minDim = std::min(w, h);

  int xOffset = 0;
  int yOffset = 0;

  if (w > h) {
    xOffset = (w - h) / 2;
  } else {
    yOffset = (h - w) / 2;
  }

  for (int i = 0; i < (minDim * minDim); i++) {

    int x = i % minDim;
    int y = i / minDim;

    float u = (x + 0.5f) / minDim;
    float v = (y + 0.5f) / minDim;

    int dstIndex = ((y + yOffset) * w) + (x + xOffset);

    heightMap[dstIndex] = std::sin(v * M_PI) * std::sin(u * M_PI) * maxHeight;
  }
}

} // namespace

int
main()
{
  const int w = 512;
  const int h = 512;

  const float maxHeight = 200;
  const float metersPerX = 1000 / w;
  const float metersPerY = 1000 / h;

  std::vector<float> heightMap(w * h);

  std::vector<float> water(w * h);

  GenHeightMap(w, h, heightMap, maxHeight);

  SavePNG("before.png", w, h, heightMap, maxHeight);

  const int iterations = 1024;

  auto getHeight = [&heightMap, w](int x, int y) {
    return heightMap[(y * w) + x];
  };

  auto getWater = [&water, w](int x, int y) { return water[(y * w) + x]; };

  auto addWater = [&water, w](int x, int y, float waterDelta) -> float {
    return water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + waterDelta);
  };

  auto addHeight = [&heightMap, w](int x, int y, float deltaHeight) {
    heightMap[(y * w) + x] += deltaHeight;
  };

  auto carryCapacity = [](int, int) -> float { return 0.01; };

  auto deposition = [](int, int) -> float { return 0.1; };

  auto erosion = [](int, int) -> float { return 0.1; };

  auto evaporation = [](int, int) -> float { return 0.1; };

  std::seed_seq seed{ 1234, 42, 4321 };

  std::mt19937 rng(seed);

  std::uniform_real_distribution<float> waterDist(1.0, 0.98);

  int rainfalls = 64;

  for (int j = 0; j < rainfalls; j++) {

    std::cout << "Simulating rainfall " << j << " of " << rainfalls
              << std::endl;

    TinyErode::Simulation simulation(w, h);

    std::transform(
      water.begin(),
      water.end(),
      water.begin(),
      [&rng, &waterDist](float) -> float { return waterDist(rng); });

    simulation.SetMetersPerX(metersPerX);
    simulation.SetMetersPerY(metersPerY);

    for (int i = 0; i < iterations; i++) {

      simulation.ComputeFlowAndTilt(getHeight, getWater);

      simulation.TransportWater(addWater);

      simulation.TransportSediment(carryCapacity,
                                   deposition,
                                   erosion,
                                   addHeight);

      simulation.Evaporate(addWater, evaporation);
    }

    simulation.TerminateRainfall(addHeight);
  }

  SavePNG("after.png", w, h, heightMap, maxHeight);

  return 0;
}

namespace {

bool
SavePNG(const char* imagePath,
        int w,
        int h,
        const std::vector<float>& heightMap,
        float maxHeight)
{
  std::vector<unsigned char> buf(w * h);

  for (int i = 0; i < (w * h); i++) {

    float v = (heightMap[i] / maxHeight) * 255.0f;

    v = std::max(std::min(v, 255.0f), 0.0f);

    buf[i] = v;
  }

  return !!stbi_write_png(imagePath, w, h, 1, buf.data(), w);
}

} // namespace
