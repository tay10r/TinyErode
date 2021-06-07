#pragma once

#include <array>
#include <vector>

class Debugger
{
public:
  static auto GetInstance() -> Debugger&;

  virtual ~Debugger() = default;

  virtual void SaveAll() const = 0;

  virtual void EnableWaterLog() = 0;

  virtual void EnableSedimentLog() = 0;

  virtual void LogWater(const std::vector<float>& water, int w, int h) = 0;

  virtual void LogSediment(const std::vector<float>& sediment,
                           int w,
                           int h) = 0;
};
