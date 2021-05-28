#pragma once

#include <array>
#include <vector>

class debugger
{
public:
  static auto get_instance() -> debugger&;

  virtual ~debugger() = default;

  virtual void save_all() const = 0;

  virtual void log_water(std::vector<float>&& water, int w, int h) = 0;

  virtual void log_flow(std::vector<std::array<float, 4>>&& flow,
                        int w,
                        int h) = 0;
};
