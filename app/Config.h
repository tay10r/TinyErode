#pragma once

#include <string>

struct Config final
{
  /// If non-empty, use this instead of noise.
  std::string imagePath;

  bool image16Bit{ true };

  int seed{ 0 };

  float noiseFrequency{ 0.1F };

  int width{ 256 };

  int height{ 256 };

  int speedup{ 50 };

  float maxElevation{ 50.0F };

  float distancePerCell{ 10 };

  float pipeRadius{ 1.0F };

  float gravity{ 9.8F };

  float timeDelta{ 0.001F };

  float kc{ 1.0F };

  float kd{ 1.0F };

  float ke{ 1.0F };

  float minTilt{ 0.01F };
};
