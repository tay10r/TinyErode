#include "landbrush.h"

#include <math.h>
#include <string.h>

#include <unordered_map>

#include <iostream>

namespace landbrush {

namespace {

constexpr float pi{ 3.141592653589793F };

namespace cpu {

class texture final : public landbrush::texture
{
public:
  texture(const uint16_t w, const uint16_t h, const format format)
    : m_format(format)
    , m_size{ w, h }
    , m_data(new float[static_cast<size_t>(m_size.total()) * static_cast<size_t>(format)])
  {
    memset(m_data.get(), 0, static_cast<size_t>(m_size.total()) * sizeof(float) * static_cast<size_t>(format));
  }

  auto get_format() const -> format override { return m_format; }

  auto get_size() const -> size override { return m_size; }

  auto get_data() -> float* { return m_data.get(); }

  auto get_data() const -> const float* { return m_data.get(); }

  void read(float* data) const override
  {
    memcpy(data, m_data.get(), sizeof(float) * static_cast<size_t>(m_size.total()));
  }

  void write(const float* data) override
  {
    memcpy(m_data.get(), data, sizeof(float) * static_cast<size_t>(m_size.total()));
  }

private:
  format m_format{ format::c1 };

  size m_size{ 0, 0 };

  std::unique_ptr<float[]> m_data;
};

class shader_base : public shader
{
public:
  auto set_float(const char* name, const float value) -> bool override
  {
    auto it = float_params_.find(name);
    if (it == float_params_.end()) {
      return false;
    }
    *it->second = value;
    return true;
  }

  auto set_texture(const char* name, landbrush::texture* tex) -> bool override
  {
    auto it = texture_params_.find(name);
    if (it == texture_params_.end()) {
      return false;
    }
    // Note: we assume here that this is a instance of a CPU texture.
    *it->second = static_cast<texture*>(tex);
    return true;
  }

  auto invoke() -> bool override
  {
    // Check that texture was actually set.
    for (auto& p : texture_params_) {
      if (p.second == nullptr) {
        return false;
      }
    }

    invoke_impl();

    // Textures are explicitly cleared to avoid use-after-free conditions.
    for (auto& p : texture_params_) {
      *p.second = nullptr;
    }

    return true;
  }

protected:
  virtual void invoke_impl() = 0;

  void add_float_param(const char* name, float* ptr) { float_params_.emplace(name, ptr); }

  void add_texture_param(const char* name, texture** tex) { texture_params_.emplace(name, tex); }

private:
  std::unordered_map<std::string, float*> float_params_;

  std::unordered_map<std::string, texture**> texture_params_;
};

class brush_shader final : public shader_base
{
public:
  brush_shader()
  {
    add_float_param("radius", &radius_);
    add_float_param("x", &x_);
    add_float_param("y", &y_);
    add_float_param("pipe_length", &distance_per_cell_);
    add_texture_param("output", &output_);
  }

protected:
  void invoke_impl() override
  {
    float* data = output_->get_data();
    const auto size = output_->get_size();
    const auto num_cells = size.total();
    const auto brush_radius2 = radius_ * radius_;

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for (uint32_t i = 0; i < num_cells; i++) {

      // pixel space
      const auto xi = i % size.width;
      const auto yi = i / size.width;

      // grid space
      const auto x = static_cast<float>(xi) * distance_per_cell_;
      const auto y = static_cast<float>(yi) * distance_per_cell_;

      const auto dx = x - x_;
      const auto dy = y - y_;
      const auto r = dx * dx + dy * dy;

      float value{ 1.0F };

      if (r > brush_radius2) {
        value = 0.0F;
      }

      data[i] = value;
    }
  }

private:
  float radius_{ 100.0F };

  float distance_per_cell_{ 10.0F };

  float x_{ 0.0F };

  float y_{ 0.0F };

  texture* output_{};
};

class blend_shader final : public shader_base
{
public:
  blend_shader()
  {
    add_float_param("k_alpha", &k_alpha_);
    add_float_param("k_beta", &k_beta_);

    add_texture_param("alpha", &alpha_);
    add_texture_param("beta", &beta_);
    add_texture_param("gamma", &gamma_);
  }

protected:
  void invoke_impl() override
  {
    const auto* alpha = alpha_->get_data();
    const auto* beta = beta_->get_data();
    auto* gamma = gamma_->get_data();

    const auto s = alpha_->get_size();
    const auto len = static_cast<size_t>(s.total()) * static_cast<size_t>(alpha_->get_format());

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for (uint32_t i = 0u; i < len; i++) {
      gamma[i] = alpha[i] * k_alpha_ + beta[i] * k_beta_;
    }
  }

private:
  float k_alpha_{ 0.5F };

  float k_beta_{ 0.5F };

  texture* alpha_{};

  texture* beta_{};

  texture* gamma_{};
};

class flux_shader final : public shader_base
{
public:
  flux_shader()
  {
    add_float_param("gravity", &gravity_);
    add_float_param("pipe_length", &pipe_length_);
    add_float_param("pipe_radius", &pipe_radius_);
    add_float_param("dt", &time_delta_);

    add_texture_param("water", &water_);
    add_texture_param("rock", &rock_);
    add_texture_param("soil", &soil_);
    add_texture_param("last_flux", &last_flux_);
    add_texture_param("next_flux", &next_flux_);
  }

protected:
  void invoke_impl() override
  {
    const auto* rock = rock_->get_data();
    const auto* soil = soil_->get_data();
    const auto* water = water_->get_data();
    const auto* last = last_flux_->get_data();
    auto* next = next_flux_->get_data();

    const auto s = next_flux_->get_size();
    const auto nul_cells = s.total();

    const float pipe_csa = pipe_radius_ * pipe_radius_ * pi;
    const auto alpha = time_delta_ * pipe_csa * gravity_ / pipe_length_;
    const auto beta = pipe_length_ * pipe_length_ / time_delta_;

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for (uint32_t i = 0; i < nul_cells; i++) {
      const auto x = i % s.width;
      const auto y = i / s.width;

      const float w_center = water[i];
      const float h_center = rock[i] + soil[i] + w_center;

      float h[4]{ h_center, h_center, h_center, h_center };

      if (x > 0) {
        const uint32_t j = i - 1;
        h[0] = rock[j] + soil[j] + water[j];
      }

      if (y > 0) {
        const uint32_t j = i - s.width;
        h[1] = rock[j] + soil[j] + water[j];
      }

      if (x < (s.width - 1)) {
        const uint32_t j = i + 1;
        h[2] = rock[j] + soil[j] + water[j];
      }

      if (y < (s.height - 1)) {
        const uint32_t j = i + s.width;
        h[3] = rock[j] + soil[j] + water[j];
      }

      float f[4];

      for (uint32_t j = 0; j < 4; j++) {
        f[j] = fmaxf(alpha * (h_center - h[j]) + last[static_cast<size_t>(i) * 4u + j], 0.0F);
      }

      const float k = fminf(1.0F, w_center * beta / (f[0] + f[1] + f[2] + f[3]));

      for (uint32_t j = 0; j < 4; j++) {
        next[static_cast<size_t>(i) * 4 + j] = f[j] * k;
      }
    }
  }

private:
  texture* rock_{};

  texture* soil_{};

  texture* water_{};

  texture* last_flux_{};

  texture* next_flux_{};

  float gravity_{ 9.8f };

  float pipe_length_{ 10.0F };

  float pipe_radius_{ 1.0F };

  float time_delta_{ 1.0e-3F };
};

class flow_shader final : public shader_base
{
public:
  flow_shader()
  {
    add_float_param("dt", &time_delta_);
    add_float_param("pipe_length", &pipe_length_);

    add_texture_param("flux", &flux_);
    add_texture_param("last_water", &last_water_);
    add_texture_param("next_water", &next_water_);
  }

protected:
  void invoke_impl() override
  {
    const auto* flux = flux_->get_data();
    const auto* last = last_water_->get_data();
    auto* next = next_water_->get_data();

    const auto s = last_water_->get_size();
    const auto nul_cells = s.width * s.height;

    const auto alpha = time_delta_ / pipe_length_;

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for (auto i = 0; i < nul_cells; i++) {

      const auto x = i % s.width;
      const auto y = i / s.width;

      const float* out = flux + i * 4;

      float in[4]{ 0, 0, 0, 0 };

      if (x > 0) {
        in[0] = flux[(i - 1) * 4 + 2];
      }

      if (y > 0) {
        in[1] = flux[(i - s.width) * 4 + 3];
      }

      if (x < (s.width - 1)) {
        in[2] = flux[(i + 1) * 4];
      }

      if (y < (s.height - 1)) {
        in[3] = flux[(i + s.width) * 4 + 1];
      }

      const float v = ((in[0] - out[0]) + (in[1] - out[1]) + (in[2] - out[2]) + (in[3] - out[3])) * alpha;

      next[i] = fmaxf(last[i] + v, 0.0F);
    }
  }

private:
  float time_delta_{ 1.0e-3F };

  float pipe_length_{ 10.0F };

  texture* flux_{};

  texture* last_water_{};

  texture* next_water_{};
};

class hydraulic_erosion_shader final : public shader_base
{
public:
  hydraulic_erosion_shader()
  {
    add_float_param("carry_capacity", &kc_);
    add_float_param("deposition", &kd_);
    add_float_param("erosion", &ke_);
    add_float_param("dt", &time_delta_);
    add_float_param("min_tilt", &min_tilt_);

    add_texture_param("flux", &flux_);
    add_texture_param("rock", &rock_);
    add_texture_param("last_soil", &last_soil_);
    add_texture_param("next_soil", &next_soil_);
    add_texture_param("last_water", &last_water_);
    add_texture_param("next_water", &next_water_);
    add_texture_param("last_sediment", &last_sediment_);
    add_texture_param("next_sediment", &next_sediment_);
  }

protected:
  void invoke_impl() override
  {
    const auto* flux = flux_->get_data();
    const auto* water0 = last_water_->get_data();
    const auto* water1 = next_water_->get_data();
    const auto* soil0 = last_soil_->get_data();
    auto* soil1 = next_soil_->get_data();
    const auto* sediment0 = last_sediment_->get_data();
    auto* sediment1 = next_sediment_->get_data();

    const auto s = last_sediment_->get_size();
    const auto num_cells = s.total();

#ifdef _OPENMP
#pragma omp parallel for
#endif

    for (uint32_t i = 0; i < num_cells; i++) {

      const auto x = i % s.width;
      const auto y = i / s.width;

      float outflow[4]{ flux[0], flux[1], flux[2], flux[3] };

      float inflow[4]{ 0, 0, 0, 0 };

      if (x > 0) {
        inflow[0] = flux[(i - 1) * 4 + 2];
      }

      if (y > 0) {
        inflow[1] = flux[(i - s.width) * 4 + 3];
      }

      if (x < (s.width - 1)) {
        inflow[2] = flux[(i + 1) * 4];
      }

      if (y < (s.height - 1)) {
        inflow[3] = flux[(i + s.width) * 4 + 1];
      }

      const float netflow[2]{ ((inflow[0] - outflow[0]) + (outflow[2] - inflow[2])) * 0.5F,
                              ((inflow[1] - outflow[1]) + (outflow[3] - inflow[3])) * 0.5F };

      const float h = 2.0F / (pipe_length_ * water0[i] * water1[i]);

      const float velocity[2]{ netflow[0] * h, netflow[1] * h };
      const float velocity_mag = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]);

      const float tilt = std::max(/*TODO->*/ 0.0F, min_tilt_);

      const float c = kc_ * tilt * velocity_mag;

      float delta_soil{ 0.0F };

      const auto s0 = sediment0[i];

      if (c > s0) {
        // erode
        delta_soil = ke_ * (s0 - c);
      } else {
        // deposit
        delta_soil = kd_ * (s0 - c);
      }

      std::cout << delta_soil << std::endl;

      soil1[i] = soil0[i] + delta_soil;

      sediment1[i] = s0 - delta_soil;
    }
  }

private:
  texture* flux_{};

  texture* rock_{};

  texture* last_soil_{};

  texture* next_soil_{};

  texture* last_water_{};

  texture* next_water_{};

  texture* last_sediment_{};

  texture* next_sediment_{};

  float kc_{ 1 };

  float kd_{ 1 };

  float ke_{ 1 };

  float time_delta_{ 1.0e-3F };

  float pipe_length_{ 10.0F };

  float min_tilt_{ 0.1F };
};

class hydraulic_transport_shader final : public shader_base
{
public:
  hydraulic_transport_shader()
  {
    //
  }

protected:
  void invoke_impl() override
  {
    //
  }

private:
};

class cpu_backend final : public landbrush::cpu_backend
{
public:
  auto create_texture(const uint16_t w, const uint16_t h, const format format)
    -> std::shared_ptr<landbrush::texture> override
  {
    return std::make_shared<texture>(w, h, format);
  }

  auto get_shader(const char* name) -> shader* override
  {
    if (strcmp(name, "flux") == 0) {
      return &flux_shader_;
    } else if (strcmp(name, "flow") == 0) {
      return &flow_shader_;
    } else if (strcmp(name, "blend") == 0) {
      return &blend_shader_;
    } else if (strcmp(name, "brush") == 0) {
      return &brush_shader_;
    } else if (strcmp(name, "hydraulic_erosion") == 0) {
      return &hydraulic_erosion_shader_;
    } else if (strcmp(name, "hydraulic_transport") == 0) {
      return &hydraulic_transport_shader_;
    }

    return nullptr;
  }

private:
  brush_shader brush_shader_;

  blend_shader blend_shader_;

  flux_shader flux_shader_;

  flow_shader flow_shader_;

  hydraulic_erosion_shader hydraulic_erosion_shader_;

  hydraulic_transport_shader hydraulic_transport_shader_;
};

} // namespace

} // namespace cpu

auto
same_size(const texture& t1, const texture& t2) -> bool
{
  const auto s1 = t1.get_size();
  const auto s2 = t2.get_size();
  return (s1.width == s2.width) && (s1.height == s2.height);
}

auto
cpu_backend::create() -> std::shared_ptr<cpu_backend>
{
  return std::make_shared<cpu::cpu_backend>();
}

} // namespace landbrush
