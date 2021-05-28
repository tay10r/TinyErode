#pragma once

#ifndef ERODE_HPP_INCLUDED
#define ERODE_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <numeric>
#include <vector>

#include <stdint.h>

namespace erode {

/// Used to associate an integer type with an equal-sized floating point type.
/// Assumes single precision is 'float' and double precision is 'double'.
template<typename float_type>
struct float_to_int final
{};

template<>
struct float_to_int<float>
{
  using int_type = int32_t;
};

template<>
struct float_to_int<double>
{
  using int_type = int64_t;
};

/// Users must implement this class (search C++ CRTP) in order to make their
/// terrain model compatible with this library.
template<typename float_type, typename derived>
class terrain_model
{
public:
  using int_type = typename float_to_int<float_type>::int_type;

  float_type get_height_at(int_type x, int_type y) const noexcept
  {
    return get_derived().get_height_at_impl(x, y);
  }

  void set_height_at(int_type x, int_type y, float_type h) noexcept
  {
    get_derived().set_height_at_impl(x, y, h);
  }

  auto width() const noexcept -> int_type { return get_derived().width_impl(); }

  auto height() const noexcept -> int_type
  {
    return get_derived().height_impl();
  }

private:
  derived& get_derived() noexcept { return static_cast<derived&>(*this); }

  const derived& get_derived() const noexcept
  {
    return static_cast<const derived&>(*this);
  }
};

template<typename scalar>
struct vec2 final
{
  scalar x, y;
};

/// Contains the water height and the flow rate (in terms of volume per unit
/// time). Used for temporary simulation data.
template<typename float_type>
struct fluid_cell final
{
  /// The water level within the cell, as a height value.
  float_type water = 0;

  /// The outwards flow rate of water in the cell. The first value is the flow
  /// rate going upwards in the grid, the second and third are left and right
  /// (respectfully), and the fourth is the flow rate going downwards within the
  /// grid.
  std::array<float_type, 4> flow_rate{ 0, 0, 0, 0 };

  float_type compute_scaling_factor(float_type time_step) const noexcept;

  float_type flow_rate_sum() const noexcept
  {
    return std::accumulate(flow_rate.begin(), flow_rate.end(), float_type(0));
  }
};

template<typename float_type, typename derived_terrain_model>
class simulator
{
public:
  using fluid_cell_type = fluid_cell<float_type>;

  using base_terrain_type = terrain_model<float_type, derived_terrain_model>;

  using int_type = typename float_to_int<float_type>::int_type;

  simulator(base_terrain_type& terrain)
    : m_terrain(terrain)
    , m_grid(m_terrain.width() * m_terrain.height())
  {}

  template<typename rng, template<typename> class dist>
  void rain(rng& r, dist<float>& water_dist)
  {
    for (auto& c : m_grid) {
      c.water = water_dist(r);
    }
  }

  void run_iteration()
  {
    update_flow_rate();

    update_water();
  }

  auto get_water_levels() const -> std::vector<float_type>
  {
    std::vector<float_type> w(width() * height());

    auto to_water = [](const fluid_cell_type& c) -> float_type {
      return c.water;
    };

    std::transform(m_grid.begin(), m_grid.end(), w.begin(), to_water);

    return w;
  }

private:
  auto width() const noexcept -> int_type { return m_terrain.width(); }

  auto height() const noexcept -> int_type { return m_terrain.height(); }

  void update_water()
  {
    for (int_type y = 0; y < height(); y++) {
      for (int_type x = 0; x < width(); x++) {
        update_water_at(x, y);
      }
    }
  }

  void update_water_at(int_type x, int_type y)
  {
    auto& center_cell = get_cell(x, y);

    auto inflow = get_total_inflow_rate(x, y);

    auto volume_delta = (inflow - center_cell.flow_rate_sum()) * m_time_step;

    float l_x = 1;

    float l_y = 1;

    center_cell.water += volume_delta / (l_x * l_y);
  }

  float get_total_inflow_rate(int center_x, int center_y) const noexcept
  {
    std::array<int_type, 4> x_deltas{ { 0, -1, 1, 0 } };
    std::array<int_type, 4> y_deltas{ { -1, 0, 0, 1 } };

    float_type total_inflow_rate = 0;

    for (int_type i = 0; i < 4; i++) {

      int_type x = center_x + x_deltas[i];
      int_type y = center_y + y_deltas[i];

      if ((x >= 0) && (x < width()) && (y >= 0) && (y < height()))
        total_inflow_rate += get_cell(x, y).flow_rate[3 - i];
    }

    return total_inflow_rate;
  }

  void update_flow_rate()
  {
    for (int_type y = 0; y < height(); y++) {
      for (int_type x = 0; x < width(); x++) {
        update_flow_rate_at(x, y);
      }
    }
  }

  void update_flow_rate_at(int_type center_x, int_type center_y)
  {
    auto& center_cell = m_grid[(center_y * width()) + center_x];

    // set up for : up, left, right, down
    std::array<int_type, 4> x_deltas{ { 0, -1, 1, 0 } };
    std::array<int_type, 4> y_deltas{ { -1, 0, 0, 1 } };

    for (int_type i = 0; i < 4; i++) {

      auto x = center_x + x_deltas[i];
      auto y = center_y + y_deltas[i];

      if ((x < 0) || (x >= width()))
        continue;

      if ((y < 0) || (y >= height()))
        continue;

      auto h_diff = total_height(center_x, center_y) - total_height(x, y);

      // The cross sectional area of the virtual pipe
      constexpr float_type area = 1;

      // The length of the virtual pipe
      constexpr float_type pipe_length = 1;

      auto c = m_time_step * area * ((m_gravity * h_diff) / pipe_length);

      center_cell.flow_rate[i] += c;

      constexpr float_type zero(0);

      center_cell.flow_rate[i] = std::max(zero, center_cell.flow_rate[i]);
    }

    auto scale = center_cell.compute_scaling_factor(m_time_step);

    for (auto& flow_rate : center_cell.flow_rate)
      flow_rate *= scale;
  }

  auto get_cell(int_type x, int_type y) const noexcept -> const fluid_cell_type&
  {
    return m_grid[(y * width()) + x];
  }

  auto get_cell(int_type x, int_type y) noexcept -> fluid_cell_type&
  {
    return m_grid[(y * width()) + x];
  }

  float_type total_height(int_type x, int_type y) const noexcept
  {
    return m_terrain.get_height_at(x, y) + get_cell(x, y).water;
  }

private:
  terrain_model<float_type, derived_terrain_model>& m_terrain;

  std::vector<fluid_cell_type> m_grid;

  float_type m_gravity = 9.8f;

  float_type m_time_step = 0.05;
};

template<typename float_type, typename derived_terrain_model>
auto
make_simulator(terrain_model<float_type, derived_terrain_model>& model)
  -> simulator<float_type, derived_terrain_model>
{
  return simulator<float_type, derived_terrain_model>(model);
}

// Implementation details beyond this point.

template<typename float_type>
float_type
fluid_cell<float_type>::compute_scaling_factor(float_type t_step) const noexcept
{
  float_type l_x = 1, l_y = 1;

  float_type total_volume = flow_rate_sum() * t_step;

  if (total_volume == float_type(0))
    return float_type(1);

  return std::min(float_type(1), (water * l_x * l_y) / total_volume);
}

} // namespace erode

#endif // ERODE_HPP_INCLUDED
