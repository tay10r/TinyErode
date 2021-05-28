#include <erode.hpp>

#include "stb_image.h"
#include "stb_image_write.h"

#include "debug.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include <math.h>

using namespace erode;

template<typename float_type>
class height_map final
  : public terrain_model<float_type, height_map<float_type>>
{
public:
  using base_type = terrain_model<float_type, height_map<float_type>>;

  template<typename int_type>
  void resize(int_type w, int_type h)
  {
    m_data.resize(w * h);
    m_width = w;
    m_height = h;
  }

  template<typename int_type>
  float_type get_height_at_impl(int_type x, int_type y) const noexcept
  {
    return m_data[(y * m_width) + x];
  }

  template<typename int_type>
  void set_height_at_impl(int_type x, int_type y, float_type h) noexcept
  {
    m_data[(y * m_width) + x] = h;
  }

  int width_impl() const noexcept { return m_width; }

  int height_impl() const noexcept { return m_height; }

  void normalize_height()
  {
    auto min_max = std::minmax_element(m_data.begin(), m_data.end());

    auto min = *min_max.first;

    auto range = *min_max.second - min;

    auto normalizer = [min, range](float h) -> float {
      return (h - min) / range;
    };

    std::transform(m_data.begin(), m_data.end(), m_data.begin(), normalizer);
  }

private:
  std::vector<float_type> m_data;
  int m_width = 0;
  int m_height = 0;
};

bool
load_image(height_map<float>& h_map, const char* path)
{
  int w = 0;
  int h = 0;

  unsigned char* buf = stbi_load(path, &w, &h, nullptr, 1);
  if (!buf)
    return false;

  h_map.resize(w, h);

  for (int i = 0; i < (w * h); i++)
    h_map.set_height_at(i % w, i / w, float(buf[i]) / 255.0f);

  free(buf);

  return true;
}

int
main(int argc, char** argv)
{
  const char* input_path = "input.png";

  if (argc > 1)
    input_path = argv[1];

  height_map<float> h_map;

  if (!load_image(h_map, input_path)) {
    std::cerr << "Failed to open '" << input_path << "'." << std::endl;
    return false;
  }

  auto w = h_map.width();
  auto h = h_map.height();

  auto s = make_simulator(h_map);

  std::seed_seq seed{ 1234, 42, 4321 };

  std::mt19937 rng(seed);

  std::uniform_real_distribution<float> water_dist(0.0, 0.1);

  s.rain(rng, water_dist);

  for (int i = 0; i < 1024; i++) {

    std::cout << "iteration " << i << std::endl;

    s.run_iteration();

    auto water = s.get_water_levels();

    debugger::get_instance().log_water(std::move(water), w, h);
  }

  h_map.normalize_height();

  std::vector<unsigned char> outBuf(h_map.width() * h_map.height());

  for (int i = 0; i < (h_map.width() * h_map.height()); i++)
    outBuf[i] = h_map.get_height_at(i % w, i / w) * 255;

  stbi_write_png("result.png", w, h, 1, outBuf.data(), w);

  debugger::get_instance().save_all();

  return EXIT_SUCCESS;
}
