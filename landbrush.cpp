#include "landbrush.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#include <unordered_map>
#include <vector>

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
      assert(*p.second != nullptr);
      if (*p.second == nullptr) {
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

      const float avg_water = (water0[i] + water1[1]) * 0.5F;

      const float h = (avg_water > 1.0e-4F) ? (1.0F / (pipe_length_ * avg_water)) : 0.0F;

      const float velocity[2]{ netflow[0] * h, netflow[1] * h };
      const float velocity_mag = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]);

      const float tilt = std::max(/*TODO->*/ 0.0F, min_tilt_);

      const float c = kc_ * tilt * velocity_mag;

      float delta_soil{ 0.0F };

      const auto current_sediment = sediment0[i];

      const auto current_soil = soil0[i];

      if (c > current_sediment) {
        // erode (but not more than there is soil)
        delta_soil = fmaxf(ke_ * (current_sediment - c), -current_soil);
      } else {
        // deposit
        delta_soil = kd_ * (current_sediment - c);
      }

      soil1[i] = current_soil + delta_soil;

      sediment1[i] = current_sediment - delta_soil;
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
    add_float_param("dt", &time_delta_);
    add_float_param("pipe_length", &pipe_length_);

    add_texture_param("flux", &flux_);
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

      const float avg_water = (water0[i] + water1[1]) * 0.5F;

      const float h = (avg_water > 1.0e-4F) ? (1.0F / (pipe_length_ * avg_water)) : 0.0F;

      const float velocity[2]{ netflow[0] * h, netflow[1] * h };

      const float xx = static_cast<float>(x) * pipe_length_ + velocity[0] * time_delta_;
      const float yy = static_cast<float>(y) * pipe_length_ + velocity[1] * time_delta_;

      const int x0 = static_cast<int>(xx);
      const int y0 = static_cast<int>(yy);

      // TODO : interpolate
      float tmp_s{ 0.0F };
      if ((x0 >= 0) && (x0 < s.width) && (y0 >= 0) && (y0 < s.height)) {
        tmp_s = sediment0[y0 * s.width + x];
      }
      sediment1[i] = tmp_s;
    }
  }

private:
  float time_delta_{ 1.0e-3F };

  float pipe_length_{ 10.0F };

  texture* flux_{};

  texture* last_water_{};

  texture* next_water_{};

  texture* last_sediment_{};

  texture* next_sediment_{};
};

class cpu_backend final : public landbrush::cpu_backend
{
public:
  auto create_texture(const uint16_t w, const uint16_t h, const format format)
    -> std::unique_ptr<landbrush::texture> override
  {
    return std::make_unique<texture>(w, h, format);
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

pipeline::pipeline(backend& bck, const uint16_t w, const uint16_t h, const float* rock, const float initial_soil_height)
{
  flux_.current() = bck.create_texture(w, h, format::c4);
  flux_.next() = bck.create_texture(w, h, format::c4);

  rock_ = bck.create_texture(w, h, format::c1);

  if (rock != nullptr) {
    rock_->write(rock);
  }

  soil_.current() = bck.create_texture(w, h, format::c1);
  soil_.next() = bck.create_texture(w, h, format::c1);

  water_.current() = bck.create_texture(w, h, format::c1);
  water_.next() = bck.create_texture(w, h, format::c1);

  sediment_.current() = bck.create_texture(w, h, format::c1);
  sediment_.next() = bck.create_texture(w, h, format::c1);

  height_ = bck.create_texture(w, h, format::c1);

  brush_ = bck.create_texture(w, h, format::c1);

  blend_shader_ = bck.get_shader("blend");
  brush_shader_ = bck.get_shader("brush");
  flux_shader_ = bck.get_shader("flux");
  flow_shader_ = bck.get_shader("flow");
  hydraulic_erosion_shader_ = bck.get_shader("hydraulic_erosion");
  hydraulic_transport_shader_ = bck.get_shader("hydraulic_transport");

  {
    std::vector<float> soil_data(static_cast<size_t>(w) * static_cast<size_t>(h), initial_soil_height);
    soil_.current()->write(soil_data.data());
  }

  sync_height();
}

auto
pipeline::get_config() const -> const config*
{
  return &config_;
}

auto
pipeline::get_config() -> config*
{
  return &config_;
}

void
pipeline::apply_water_brush(float x, float y, float r)
{
  brush_shader_->set_float("x", x);
  brush_shader_->set_float("y", y);
  brush_shader_->set_float("radius", r);
  brush_shader_->set_float("pipe_length", config_.pipe_length);
  brush_shader_->set_texture("output", brush_.get());
  brush_shader_->invoke();

  blend_shader_->set_float("k_alpha", 1.0F);
  blend_shader_->set_float("k_beta", 1.0F);
  blend_shader_->set_texture("alpha", water_.current().get());
  blend_shader_->set_texture("beta", brush_.get());
  blend_shader_->set_texture("gamma", water_.next().get());
  blend_shader_->invoke();

  water_.step();
}

void
pipeline::add_uniform_water(const float delta_water_height)
{
  {
    const auto s = brush_->get_size();
    std::vector<float> data(s.total(), delta_water_height);
    brush_->write(data.data());
  }

  blend_shader_->set_float("k_alpha", 1.0F);
  blend_shader_->set_float("k_beta", 1.0F);
  blend_shader_->set_texture("alpha", water_.current().get());
  blend_shader_->set_texture("beta", brush_.get());
  blend_shader_->set_texture("gamma", water_.next().get());
  blend_shader_->invoke();

  water_.step();
}

void
pipeline::step()
{
  for (uint32_t i = 0; i < config_.iterations_per_step; i++) {

    flux_shader_->set_float("gravity", config_.gravity);
    flux_shader_->set_float("pipe_length", config_.pipe_length);
    flux_shader_->set_float("pipe_radius", config_.pipe_radius);
    flux_shader_->set_float("dt", config_.time_delta);
    flux_shader_->set_texture("rock", rock_.get());
    flux_shader_->set_texture("soil", soil_.current().get());
    flux_shader_->set_texture("water", water_.current().get());
    flux_shader_->set_texture("last_flux", flux_.current().get());
    flux_shader_->set_texture("next_flux", flux_.next().get());
    flux_shader_->invoke();

    flux_.step();

    flow_shader_->set_float("dt", config_.time_delta);
    flow_shader_->set_float("pipe_length", config_.pipe_length);
    flow_shader_->set_texture("flux", flux_.current().get());
    flow_shader_->set_texture("last_water", water_.current().get());
    flow_shader_->set_texture("next_water", water_.next().get());
    flow_shader_->invoke();

    hydraulic_erosion_shader_->set_float("carry_capacity", config_.carry_capacity);
    hydraulic_erosion_shader_->set_float("deposition", config_.deposition);
    hydraulic_erosion_shader_->set_float("erosion", config_.erosion);
    hydraulic_erosion_shader_->set_float("dt", config_.time_delta);
    hydraulic_erosion_shader_->set_float("min_tilt", config_.min_tilt);
    hydraulic_erosion_shader_->set_texture("flux", flux_.current().get());
    hydraulic_erosion_shader_->set_texture("rock", rock_.get());
    hydraulic_erosion_shader_->set_texture("last_soil", soil_.current().get());
    hydraulic_erosion_shader_->set_texture("next_soil", soil_.next().get());
    hydraulic_erosion_shader_->set_texture("last_water", water_.current().get());
    hydraulic_erosion_shader_->set_texture("next_water", water_.next().get());
    hydraulic_erosion_shader_->set_texture("last_sediment", sediment_.current().get());
    hydraulic_erosion_shader_->set_texture("next_sediment", sediment_.next().get());
    hydraulic_erosion_shader_->invoke();

    sediment_.step();

    hydraulic_transport_shader_->set_float("dt", config_.time_delta);
    hydraulic_transport_shader_->set_float("pipe_length", config_.pipe_length);
    hydraulic_transport_shader_->set_texture("flux", flux_.current().get());
    hydraulic_transport_shader_->set_texture("last_water", water_.current().get());
    hydraulic_transport_shader_->set_texture("next_water", water_.next().get());
    hydraulic_transport_shader_->set_texture("last_sediment", sediment_.current().get());
    hydraulic_transport_shader_->set_texture("next_sediment", sediment_.next().get());
    hydraulic_transport_shader_->invoke();

    // end

    soil_.step();

    sediment_.step();

    water_.step();
  }

  sync_height();
}

void
pipeline::read_height(float* data) const
{
  height_->read(data);
}

void
pipeline::sync_height()
{
  blend_shader_->set_float("k_alpha", 1.0F);
  blend_shader_->set_float("k_beta", 1.0F);
  blend_shader_->set_texture("alpha", rock_.get());
  blend_shader_->set_texture("beta", soil_.current().get());
  blend_shader_->set_texture("gamma", height_.get());
  blend_shader_->invoke();
}

} // namespace landbrush
