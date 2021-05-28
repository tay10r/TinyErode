#include "debug.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

#include <math.h>

namespace {

bool
contains_nan(float n)
{
  return std::isnan(n);
}

bool
contains_nan(const std::array<float, 4>& flow)
{
  for (auto n : flow) {
    if (contains_nan(n))
      return true;
  }

  return false;
}

bool
contains_inf(float n)
{
  auto inf = std::numeric_limits<float>::infinity();

  return (n == inf) || (n == -inf);
}

bool
contains_inf(const std::array<float, 4>& flow)
{
  for (auto n : flow) {
    if (contains_inf(n))
      return true;
  }

  return false;
}

template<typename pixel_type>
class frame final
{
public:
  frame(std::vector<pixel_type>&& d, int w, int h)
    : m_data(std::move(d))
    , m_width(w)
    , m_height(h)
  {
    check_data();
  }

  const auto& get_data() const noexcept { return m_data; }

  int get_width() const noexcept { return m_width; }

  int get_height() const noexcept { return m_height; }

private:
  void check_data()
  {
    for (int i = 0; i < (m_width * m_height); i++) {
      if (contains_nan(m_data[i])) {
        std::cout << "NaN detected" << std::endl;
        break;
      }

      if (contains_inf(m_data[i])) {
        std::cout << "Infinity detected" << std::endl;
        break;
      }
    }
  }

private:
  std::vector<pixel_type> m_data;
  int m_width = 0;
  int m_height = 0;
};

template<typename pixel_converter>
bool
save_to_ppm(const char* path, int w, int h, pixel_converter converter)
{
  std::vector<unsigned char> buf(w * h * 3);

  for (int i = 0; i < (w * h); i++) {

    std::array<float, 3> c = converter(i);
    buf[(i * 3) + 0] = c[0] * 255;
    buf[(i * 3) + 1] = c[1] * 255;
    buf[(i * 3) + 2] = c[2] * 255;
  }

  std::ofstream file(path);
  if (!file.good())
    return false;

  file << "P6\n" << w << ' ' << h << "\n255\n";

  file.write((const char*)buf.data(), buf.size());

  return true;
}

class debugger_impl final : public debugger
{
public:
  void log_water(std::vector<float>&& data, int w, int h) override
  {
    m_water_frames.emplace_back(std::move(data), w, h);
  }

  void log_flow(std::vector<std::array<float, 4>>&& data, int w, int h) override
  {
    m_flow_frames.emplace_back(std::move(data), w, h);
  }

  void save_all() const override { save_water_frames(); }

  void save_water_frames() const
  {
    auto min_and_range = get_water_min_and_range();

    auto min = min_and_range.first;

    auto range = min_and_range.second;

    std::cout << min << ' ' << range << std::endl;

    int frame_counter = 0;

    for (const auto& f : m_water_frames) {

      std::ostringstream path_stream;
      path_stream << "water_";
      path_stream << std::setfill('0') << std::setw(4) << frame_counter;
      path_stream << ".ppm";

      auto path = path_stream.str();

      const auto& data = f.get_data();
      auto w = f.get_width();
      auto h = f.get_height();

      auto to_pixel = [&data, min, range](int index) -> std::array<float, 3> {
        auto water_level = data[index];

        auto normalized_wl = (water_level - min) / range;

        return { { normalized_wl, normalized_wl, normalized_wl } };
      };

      save_to_ppm(path.c_str(), w, h, to_pixel);

      frame_counter++;
    }
  }

  std::pair<float, float> get_water_min_and_range() const
  {
    float global_min = std::numeric_limits<float>::infinity();

    float global_max = -std::numeric_limits<float>::infinity();

    for (const auto& f : m_water_frames) {

      const auto& data = f.get_data();

      auto min_max = std::minmax_element(data.begin(), data.end());

      std::cout << *min_max.first << ' ' << *min_max.second << std::endl;

      global_min = std::min(global_min, *min_max.first);
      global_max = std::max(global_max, *min_max.second);
    }

    return { global_min, global_max - global_min };
  }

private:
  using water_frame = frame<float>;

  using flow_frame = frame<std::array<float, 4>>;

  std::vector<water_frame> m_water_frames;

  std::vector<flow_frame> m_flow_frames;
};

} // namespace

debugger&
debugger::get_instance()
{
  static debugger_impl db;
  return db;
}
