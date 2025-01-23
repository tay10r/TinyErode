#pragma once

#include <string>

struct Config final
{
  // Initial Conditions
  // ------------------

  /// If non-empty, use this instead of noise.
  std::string imagePath;

  bool image16Bit{ true };

  int seed{ 0 };

  float noiseFrequency{ 0.1F };

  int width{ 256 };

  int height{ 256 };

  float distancePerCell{ 10 };

  float rock_height{ 30.0F };

  float initial_soil_height{ 20.0F };

  // Simulation
  // ----------

  int speedup{ 5 };

  float pipeRadius{ 1.0F };

  float gravity{ 9.8F };

  float timeDelta{ 0.001F };

  float kc{ 0.02F };

  float kd{ 0.1F };

  float ke{ 0.1F };

  float minTilt{ 0.01F };

  // Brush
  // -----

  std::string brush_mode{ "Water" };
};
