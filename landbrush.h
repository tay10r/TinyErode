/* SPDX-License-Identifier: MIT
 *
 *  _                 _ _                    _
 * | |               | | |                  | |
 * | | __ _ _ __   __| | |__  _ __ _   _ ___| |__
 * | |/ _` | '_ \ / _` | '_ \| '__| | | / __| '_ \
 * | | (_| | | | | (_| | |_) | |  | |_| \__ \ | | |
 * |_|\__,_|_| |_|\__,_|_.__/|_|   \__,_|___/_| |_|
 *
 * Copyright (C) 2021-2025 Taylor Holberton
 *
 * A C++ library for modeling terrain.
 */

#pragma once

#ifndef LANDBRUSH_H_INCLUDED
#define LANDBRUSH_H_INCLUDED

#include <map>
#include <memory>
#include <string>

#include <stdint.h>
#include <stdlib.h>

namespace landbrush {

/// @brief Indicates the number of channels per cell in a texture.
enum class format : uint8_t
{
  /// @brief One channels per cell.
  c1 = 1,
  /// @brief Two channels per cell.
  c2 = 2,
  /// @brief Three channels per cell.
  c3 = 3,
  /// @brief Four channels per cell.
  c4 = 4
};

/// @brief Describes the shape of a texture.
struct size final
{
  /// @brief The number of columns in the texture.
  uint16_t width{};

  /// @brief The number of rows in the texture.
  uint16_t height{};

  /// @brief Gets the total number of cells in the texture.
  [[nodiscard]] auto total() const -> uint32_t { return static_cast<uint32_t>(width) * static_cast<uint32_t>(height); }
};

/// @brief Represents a texture living in the memory space of the compute device.
class texture
{
public:
  virtual ~texture() = default;

  /// @brief Gets the format of the texture.
  /// @return The format of the texture.
  virtual auto get_format() const -> format = 0;

  /// @brief Gets the size of the texture.
  /// @return The size of the texture.
  virtual auto get_size() const -> size = 0;

  /// @brief Reads the texture data into host memory.
  ///
  /// @note The size of the buffer must be equal the size of the texture, given the width, height, and number of
  /// features.
  virtual void read(float* data) const = 0;

  /// @brief Writes the texture data to device memory.
  ///
  /// @note The size of the buffer must be equal the size of the texture, given the width, height, and number of
  /// features.
  virtual void write(const float* data) = 0;

  /// Indicates whether or not the texture has a specified format.
  ///
  /// @param f The format to check for.
  ///
  /// @return True on success, false on failure.
  auto has_format(format f) const -> bool { return f == get_format(); }
};

/// @brief Indicates whether or not two textures are the same size.
auto
same_size(const texture& t1, const texture& t2) -> bool;

/// @brief Represents a function in the pipeline that typically operates on textures.
class shader
{
public:
  virtual ~shader() = default;

  /// @brief Sets a floating point parameter.
  ///
  /// @param name The name of the parameter to set.
  ///
  /// @param value The value to assign the parameter.
  ///
  /// @return True on success, false on failure.
  virtual auto set_float(const char* name, float value) -> bool = 0;

  /// @brief Sets a texture parameter.
  ///
  /// @param name The name of the parameter.
  ///
  /// @param tex The texture to assign.
  ///
  /// @note The pointer to the texture assigned to the parameter shall be cleared after invoking the shader or until
  ///       the parameter is assigned again with a different texture.
  virtual auto set_texture(const char* name, texture* tex) -> bool = 0;

  /// @brief Invokes the kernel.
  /// @return True on success, false on failure.
  virtual auto invoke() -> bool = 0;
};

/// @brief Represents the compute backend that will be executing the shaders of the simulation.
/// This can be derived in order to run the algorithms on different accelerators.
class backend
{
public:
  virtual ~backend() = default;

  /// @brief Creates a new texture.
  /// @param w The width of the texture.
  /// @param h The height of the texture.
  /// @param fmt The format of the texture.
  /// @return A new texture instance.
  virtual auto create_texture(uint16_t w, uint16_t h, format fmt) -> std::unique_ptr<texture> = 0;

  /// @brief Gets access to a shader, if it is supported by the backend.
  /// @param name The name of the shader to access.
  /// @return A pointer to the kernel. If the pointer is not found, a null pointer is returned.
  virtual auto get_shader(const char* name) -> shader* = 0;
};

/// @brief The built-in implementation of a backend.
///
/// @note This is mostly for reference and fallback. It works reliably, but expect the low throughput when using it.
class cpu_backend : public backend
{
public:
  /// @brief Creates a new CPU backend.
  static auto create() -> std::shared_ptr<cpu_backend>;

  ~cpu_backend() override = default;
};

/// @brief This is a swap buffer. It contains two elements where one acts as an input and the other acts as an output.
///        It is primarily useful for state transitions.
template<typename T>
class swap_buffer final
{
public:
  auto current() -> T& { return elements_[index_ % 2]; }

  auto current() const -> const T& { return elements_[index_ % 2]; }

  auto next() -> T& { return elements_[(index_ + 1) % 2]; }

  auto next() const -> const T& { return elements_[(index_ + 1) % 2]; }

  void step() { index_++; }

private:
  T elements_[2];

  uint8_t index_{};
};

/// @brief This is a high-level object providing a practical pipeline for eroding terrain.
///        It creates the required set of textures and utilizes the various shaders from
///        the backend to complete the terrain modeling pipeline.
class pipeline final
{
public:
  /// @brief Contains the data required to tweek the pipeline.
  struct config final
  {
    /// @brief The number of seconds between time iterations. The lower this number is, the more accurate and stable the
    /// pipeline is.
    float time_delta{ 0.01F };

    /// @brief The distance between cells, in terms of meters.
    float pipe_length{ 10.0F };

    /// @brief The radius of the virtual pipe connecting cells.
    ///        This can be used to increase or descrease the total rate of flow.
    float pipe_radius{ 2.0F };

    /// @brief The amount of gravity.
    float gravity{ 9.8F };

    /// @brief Affects the amount of sediment that can be carried by water.
    float carry_capacity{ 1.0F };

    /// @brief Affects how the rate at which soil can be picked up by water.
    float erosion{ 1.0F };

    /// @brief Affects how the rate at which soil can be dropped by water.
    float deposition{ 1.0F };

    /// @brief The minimum amount of tilt to assume for each cell in the terrain, when it comes to hydraulic erosion.
    float min_tilt{ 0.01F };

    /// @brief The number of time iterations per step.
    uint32_t iterations_per_step{ 16 };
  };

  /// @brief Constructs a new pipeline object.
  /// @param bck The backend being used to accelerate the computation.
  /// @param w The width of the terrain, in terms of texels.
  /// @param h The height of the terrain, in terms of texels.
  /// @param rock_data The data for the rock base layer.
  ///                  This may be null, in which case the rock base layer will remain flat.
  pipeline(backend& bck, uint16_t w, uint16_t h, const float* rock_data);

  auto get_config() -> config*;

  auto get_config() const -> const config*;

  /// @brief Applies a water brush, causing water to fall onto the terrain where the brush is located.
  /// @param x The location of the brush, in meters.
  /// @param y The location of the brush, in meters.
  /// @param radius The radius of the brush, in meters.
  void apply_water_brush(float x, float y, float radius);

  /// @brief Moves the state of the terrain forward in time.
  void step();

private:
  /// @brief The configuration object for the pipeline.
  config config_;

  /// @brief The texture containing the mask of where the brush was last applied.
  std::unique_ptr<texture> brush_;

  /// @brief The texture containing the output flux of each cell in the terrain.
  swap_buffer<std::unique_ptr<texture>> flux_;

  /// @brief The base layer of the terrain, which does not change.
  std::unique_ptr<texture> rock_;

  /// @brief The layer of the terrain which can be modified.
  swap_buffer<std::unique_ptr<texture>> soil_;

  /// @brief The water layer in the terrain, responsible for hydraulic erosion, deposition and transport.
  swap_buffer<std::unique_ptr<texture>> water_;

  /// @brief The sediment being carried by water.
  swap_buffer<std::unique_ptr<texture>> sediment_;

  /// @brief The shader for blending two textures.
  shader* blend_shader_{};

  /// @brief Used for generating brush patterns. Useful for interactive editing.
  shader* brush_shader_{};

  /// @brief The shader for computing flux.
  shader* flux_shader_{};

  /// @brief The shader for transporting water.
  shader* flow_shader_{};

  /// @brief For picking up and depositing sediment with water.
  shader* hydraulic_erosion_shader_{};

  /// @brief For transporting sediment suspended in water.
  shader* hydraulic_transport_shader_{};
};

} // namespace landbrush

#endif // LANDBRUSH_H_INCLUDED
