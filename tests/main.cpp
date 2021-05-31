#include <TinyErode.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include "debug.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include <math.h>
#include <string.h>

bool
LoadImage(std::vector<float>& heightMap, int& w, int& h, const char* path)
{
  unsigned char* buf = stbi_load(path, &w, &h, nullptr, 1);
  if (!buf)
    return false;

  heightMap.resize(w * h);

  for (int i = 0; i < (w * h); i++)
    heightMap.at(i) = float(buf[i]) / 255.0f;

  free(buf);

  return true;
}

void
Normalize(std::vector<float>& v)
{
  auto minMax = std::minmax_element(v.begin(), v.end());

  auto min = *minMax.first;
  auto max = *minMax.second;
  auto range = max - min;

  auto normalize = [min, range](float in) -> float {
    return (in - min) / range;
  };

  std::transform(v.begin(), v.end(), v.begin(), normalize);
}

int
main(int argc, char** argv)
{
  const char* inputPath = "input.png";

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--log-water") == 0) {
      Debugger::GetInstance().EnableWaterLog();
    } else if (strcmp(argv[i], "--log-sediment") == 0) {
      Debugger::GetInstance().EnableSedimentLog();
    } else if (argv[i][0] == '-') {
      std::cerr << "Unknown option '" << argv[i] << "'" << std::endl;
    } else {
      inputPath = argv[i];
    }
  }

  int w = 0;
  int h = 0;

  std::vector<float> heightMap;

  if (!LoadImage(heightMap, w, h, inputPath)) {
    std::cerr << "Failed to open '" << inputPath << "'." << std::endl;
    return false;
  }

  std::vector<float> water(w * h);

  std::seed_seq seed{ 1234, 42, 4321 };

  std::mt19937 rng(seed);

  std::uniform_real_distribution<float> waterDist(0.0, 0.1);

  for (auto& r : water)
    r = waterDist(rng);

  double totalTime = 0;

  int iterations = 256;

  TinyErode tinyErode(w, h);

  auto getWater = [&water, w](int x, int y) -> float {
    return water[(w * y) + x];
  };

  auto addWater = [&water, w](int x, int y, float dw) {
    water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + dw);
  };

  auto getHeight = [&heightMap, w](int x, int y) -> float {
    return heightMap[(w * y) + x];
  };

  auto addHeight = [&heightMap, w](int x, int y, float dh) {
    return heightMap[(w * y) + x] += dh;
  };

  auto carryCapacity = [](int, int) -> float { return 0.5; };

  auto erosion = [](int, int) -> float { return 0.1; };

  auto deposition = [](int, int) -> float { return 0.1; };

  auto evaporation = [](int, int) -> float { return 0.01; };

  for (int i = 0; i < iterations; i++) {

    std::cout << "iteration " << i << " of " << iterations << '\r';

    auto start = std::chrono::high_resolution_clock::now();

    tinyErode.ComputeFlowAndTilt(getHeight, getWater);

    tinyErode.TransportWater(addWater);

    tinyErode.TransportSediment(carryCapacity, deposition, erosion, addHeight);

    tinyErode.Evaporate(addWater, evaporation);

    auto stop = std::chrono::high_resolution_clock::now();

    auto time_delta =
      std::chrono::duration_cast<std::chrono::duration<float>>(stop - start)
        .count();

    totalTime += time_delta;

    Debugger::GetInstance().LogWater(water, w, h);

    Debugger::GetInstance().LogSediment(tinyErode.GetSediment(), w, h);
  }

  std::cout << std::endl;

  std::cout << "seconds per iteration: " << totalTime / iterations << std::endl;

  Normalize(heightMap);

  std::vector<unsigned char> outBuf(w * h);

  for (int i = 0; i < (w * h); i++)
    outBuf[i] = heightMap[i] * 255;

  stbi_write_png("result.png", w, h, 1, outBuf.data(), w);

  Debugger::GetInstance().SaveAll();

  return EXIT_SUCCESS;
}
