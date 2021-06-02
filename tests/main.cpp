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

float
Clamp(float x, float min, float max)
{
  return std::max(std::min(x, max), min);
}

void
Rain(std::vector<float>& water, std::mt19937& rng)
{
  std::uniform_real_distribution<float> waterDist(0.8, 0.9);

  for (auto& r : water)
    r += waterDist(rng);
}

bool
ParseIntOpt(const char* name, const char* arg1, const char* arg2, int* value)
{
  if (strcmp(name, arg1) != 0)
    return false;

  return sscanf(arg2, "%d", value) == 1;
}

bool
ParseFloatOpt(const char* name,
              const char* arg1,
              const char* arg2,
              float* value)
{
  if (strcmp(name, arg1) != 0)
    return false;

  return sscanf(arg2, "%f", value) == 1;
}

int
main(int argc, char** argv)
{
  const char* inputPath = "input.png";

  float timeDelta = 0.0125;

  int stepsPerRain = 16;

  float scale = 1.0f;

  float kErosion = 0.05;

  float kDeposition = 0.05;

  float kCapacity = 0.1;

  float kEvaporation = 1.0f / (stepsPerRain * timeDelta);

  int iterations = 16;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--log-water") == 0) {
      Debugger::GetInstance().EnableWaterLog();
    } else if (strcmp(argv[i], "--log-sediment") == 0) {
      Debugger::GetInstance().EnableSedimentLog();
    } else if (ParseFloatOpt("--scale", argv[i], argv[i + 1], &scale)) {
      i++;
      continue;
    } else if (ParseFloatOpt("--erosion", argv[i], argv[i + 1], &kErosion)) {
      i++;
      continue;
    } else if (ParseFloatOpt("--deposition",
                             argv[i],
                             argv[i + 1],
                             &kDeposition)) {
      i++;
      continue;
    } else if (ParseFloatOpt("--capacity", argv[i], argv[i + 1], &kCapacity)) {
      i++;
      continue;
    } else if (ParseFloatOpt("--evaporation",
                             argv[i],
                             argv[i + 1],
                             &kEvaporation)) {
      i++;
      continue;
    } else if (ParseIntOpt("--iterations", argv[i], argv[i + 1], &iterations)) {
      i++;
      continue;
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

  for (auto& value : heightMap)
    value *= scale;

  std::vector<float> water(w * h);

  std::seed_seq seed{ 1234, 42, 4321 };

  std::mt19937 rng(seed);

  double totalTime = 0;

  TinyErode tinyErode(w, h);

  auto getWater = [&water, w](int x, int y) -> float {
    return water[(w * y) + x];
  };

  auto addWater = [&water, w](int x, int y, float dw) -> float {
    return water[(y * w) + x] = std::max(0.0f, water[(y * w) + x] + dw);
  };

  auto getHeight = [&heightMap, w](int x, int y) -> float {
    return heightMap[(w * y) + x];
  };

  auto addHeight = [&heightMap, w](int x, int y, float dh) {
    // std::cout << dh << std::endl;
    return heightMap[(w * y) + x] += dh;
  };

  auto carryCapacity = [kCapacity](int, int) -> float { return kCapacity; };

  auto erosion = [kErosion](int, int) -> float { return kErosion; };

  auto deposition = [kDeposition](int, int) -> float { return kDeposition; };

  auto evaporation = [kEvaporation](int, int) -> float { return kEvaporation; };

  for (int i = 0; i < iterations; i++) {

    std::cout << "Simulating rainfall " << i << " of " << iterations
              << std::endl;

    Rain(water, rng);

    for (int j = 0; j < stepsPerRain; j++) {

      auto start = std::chrono::high_resolution_clock::now();

      tinyErode.ComputeFlowAndTilt(getHeight, getWater);

      tinyErode.TransportWater(addWater);

      tinyErode.TransportSediment(carryCapacity,
                                  deposition,
                                  erosion,
                                  addHeight);

      tinyErode.Evaporate(addWater, evaporation);

      auto stop = std::chrono::high_resolution_clock::now();

      auto time_delta =
        std::chrono::duration_cast<std::chrono::duration<float>>(stop - start)
          .count();

      totalTime += time_delta;

      Debugger::GetInstance().LogWater(water, w, h);

      Debugger::GetInstance().LogSediment(tinyErode.GetSediment(), w, h);
    }
  }

  std::cout << "Seconds per rainfall: " << totalTime / iterations << std::endl;

  Normalize(heightMap);

  std::vector<unsigned char> outBuf(w * h);

  for (int i = 0; i < (w * h); i++)
    outBuf[i] = Clamp(heightMap[i] * 255, 0.0f, 255.0f);

  stbi_write_png("result.png", w, h, 1, outBuf.data(), w);

  Debugger::GetInstance().SaveAll();

  return EXIT_SUCCESS;
}
