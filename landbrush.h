/* SPDX-License-Identifier: MIT
 *
 *  ______               _
 * |  ____|             (_)
 * | |__   _ __ ___  ___ _  ___  _ __
 * |  __| | '__/ _ \/ __| |/ _ \| '_ \
 * | |____| | | (_) \__ \ | (_) | | | |
 * |______|_|  \___/|___/_|\___/|_| |_|
 *
 * Copyright (C) 2021 Taylor Holberton
 *
 * A C++ library for simulating erosion.
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

/// @brief Represents a function in the simulation that typically operates on textures.
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
  virtual auto create_texture(uint16_t w, uint16_t h, format fmt) -> std::shared_ptr<texture> = 0;

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

} // namespace landbrush

#endif // LANDBRUSH_H_INCLUDED
