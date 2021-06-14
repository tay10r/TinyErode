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
#if 1
  float delta = 0.05;

  float center = 1.0;

  float min = center - (center * delta);
  float max = center + (center * delta);

  std::uniform_real_distribution<float> waterDist(min, max);

  for (auto& r : water)
    r = waterDist(rng);
#else
  for (auto& w : water)
    w = 0;

  std::uniform_int_distribution<int> indexDist(0, water.size() - 1);

  for (int i = 0; i < 128; i++)
    water[indexDist(rng)] += 1;
#endif
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

  int stepsPerRain = 256;

  float minHeight = 0;

  float heightRange = 200.0f;

  float kErosion = 0.005;

  float kDeposition = 0.010;

  float kCapacity = 0.01;

  float kEvaporation = 0.1;

  float xRange = 500;
  float yRange = 500;

  float timeStep = 0.0125;

  int rainfalls = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--log-water") == 0) {
      Debugger::GetInstance().EnableWaterLog();
    } else if (strcmp(argv[i], "--log-sediment") == 0) {
      Debugger::GetInstance().EnableSedimentLog();
    } else if (ParseFloatOpt("--height-range",
                             argv[i],
                             argv[i + 1],
                             &heightRange)) {
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
    } else if (ParseFloatOpt("--time-step", argv[i], argv[i + 1], &timeStep)) {
      i++;
      continue;
    } else if (ParseIntOpt("--rainfalls", argv[i], argv[i + 1], &rainfalls)) {
      i++;
      continue;
    } else if (ParseIntOpt("--steps-per-rainfall",
                           argv[i],
                           argv[i + 1],
                           &stepsPerRain)) {
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
    value = minHeight + (value * heightRange);

  std::vector<float> water(w * h);

  std::seed_seq seed{ 1234, 42, 4321 };

  std::mt19937 rng(seed);

  double totalTime = 0;

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

  for (int i = 0; i < rainfalls; i++) {

    TinyErode::Simulation simulation(w, h);

    simulation.SetTimeStep(timeStep);
    simulation.SetMetersPerX(xRange / w);
    simulation.SetMetersPerY(yRange / h);

    std::cout << "Simulating rainfall " << i << " of " << rainfalls
              << std::endl;

    Rain(water, rng);

    for (int j = 0; j < stepsPerRain; j++) {

      Debugger::GetInstance().LogWater(water, w, h);

      Debugger::GetInstance().LogSediment(simulation.GetSediment(), w, h);

      auto start = std::chrono::high_resolution_clock::now();

      simulation.ComputeFlowAndTilt(getHeight, getWater);

      simulation.TransportWater(addWater);

      simulation.TransportSediment(carryCapacity,
                                   deposition,
                                   erosion,
                                   addHeight);

      simulation.Evaporate(addWater, evaporation);

      auto stop = std::chrono::high_resolution_clock::now();

      auto time_delta =
        std::chrono::duration_cast<std::chrono::duration<float>>(stop - start)
          .count();

      totalTime += time_delta;
    }

    simulation.TerminateRainfall(addHeight);
  }

  std::cout << "Seconds per iteration: "
            << totalTime / (rainfalls * stepsPerRain) << std::endl;

  Normalize(heightMap);

  std::vector<unsigned char> outBuf(w * h);

  for (int i = 0; i < (w * h); i++)
    outBuf[i] = Clamp(heightMap[i] * 255, 0.0f, 255.0f);

  stbi_write_png("result.png", w, h, 1, outBuf.data(), w);

  Debugger::GetInstance().SaveAll();

  return EXIT_SUCCESS;
}
