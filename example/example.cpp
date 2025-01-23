/// @file example.cpp
///
/// @brief This file demonstrates basic usage of the library by generating a height map, eroding it, and then saving it
///        to a file. The initial state of the height map is also saved for comparison.

#include <landbrush.h>

#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#define _USE_MATH_DEFINES 1

#include <math.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

namespace {

/// @brief Saves an image to a PNG file.
/// @param image_path The path to save the image at.
/// @param w The width of the image, in pixels.
/// @param h The height of the image, in pixels.
/// @param height_map The height map to save.
/// @param max_height The maximum height of the height map, which is used to normalize the data.
/// @return True on success, false on failure.
auto
save_png(const char* image_path, int w, int h, const std::vector<float>& height_map, float max_height) -> bool;

/// @brief Generates a height map for testing purposes.
/// @param w The width of the height map, in pixels.
/// @param h The height of the height map, in pixels.
/// @param height_map The data to place the generated data into.
/// @param max_height The maximum height to generate.
void
gen_height_map(int w, int h, std::vector<float>& height_map, float max_height);

} // namespace

int
main()
{
  // This will be the size of the texture.
  const uint16_t w = 512;
  const uint16_t h = 512;

  // A maximum elevation of 50 meters.
  const float max_height = 50;

  // Contains the height of each cell.
  std::vector<float> height_map(static_cast<size_t>(w) * static_cast<size_t>(h));

  // Generate an initial terrain
  gen_height_map(w, h, height_map, max_height);

  // Save a copy so that we can examine how its changed.
  save_png("before.png", w, h, height_map, max_height);

  // Here we create a backend. A backend is what is used to accelerate the erosion process.
  // Ideally, we'd be using a backend that executes on a GPU, but to keep this example simple
  // we will be using the built-in CPU backend.
  auto backend = landbrush::cpu_backend::create();

  // Here we create a pipeline object. This ties together the various
  // components of the backend in order to modify the terrain.
  landbrush::pipeline pipeline(*backend, w, h, height_map.data());

  pipeline.apply_water_brush(/*x=*/0, /*y=*/0, /*radius=*/10);
  pipeline.step();

#if 0
  SavePNG("after.png", w, h, height_map, max_height);
#endif

  return 0;
}

namespace {

auto
save_png(const char* image_path, int w, int h, const std::vector<float>& height_map, float max_height) -> bool
{
  std::vector<unsigned char> buf(w * h);

  for (int i = 0; i < (w * h); i++) {

    float v = (height_map[i] / max_height) * 255.0f;

    v = std::max(std::min(v, 255.0f), 0.0f);

    buf[i] = v;
  }

  return !!stbi_write_png(image_path, w, h, 1, buf.data(), w);
}

void
gen_height_map(const int w, const int h, std::vector<float>& height_map, const float max_height)
{
  int min_dim = std::min(w, h);

  int x_offset = 0;
  int y_offset = 0;

  if (w > h) {
    x_offset = (w - h) / 2;
  } else {
    y_offset = (h - w) / 2;
  }

  const int num_spheres = 4;

  std::vector<float> sphere_u(num_spheres);
  std::vector<float> sphere_v(num_spheres);
  std::vector<float> sphere_r(num_spheres);

  const int seed = 0;

  std::mt19937 rng(seed);

  for (auto i = 0; i < num_spheres; i++) {
    std::uniform_real_distribution<float> dist(0, 1);
    sphere_u[i] = dist(rng);
    sphere_v[i] = dist(rng);

    std::uniform_real_distribution<float> r_dist(0.1, 0.4);
    sphere_r[i] = r_dist(rng);
  }

  for (int i = 0; i < (min_dim * min_dim); i++) {

    int x = i % min_dim;
    int y = i / min_dim;

    float u = (x + 0.5f) / min_dim;
    float v = (y + 0.5f) / min_dim;

    int dst_index = ((y + y_offset) * w) + (x + x_offset);

    float cell{ 0 };

    for (auto i = 0; i < num_spheres; i++) {
      const auto du = u - sphere_u[i];
      const auto dv = v - sphere_v[i];
      const auto d2 = du * du + dv * dv;
      const auto r = sphere_r[i];
      if (d2 < (r * r)) {
        const auto k = sqrtf(d2) / r;
        cell = fmaxf(std::cos(k * M_PI * 0.5F), cell);
      }
    }

    std::uniform_real_distribution<float> noise_dist(0, 2);

    height_map[dst_index] = cell * max_height + noise_dist(rng);
  }
}

} // namespace
