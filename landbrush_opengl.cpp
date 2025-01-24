#include "landbrush_opengl.h"

namespace {

#include "landbrush_opengl_internal.h"

} // namespace

namespace landbrush {

namespace {

namespace opengl {

[[nodiscard]] auto
to_gl_internal_format(const format fmt) -> GLenum
{
  switch (fmt) {
    case format::c1:
      return GL_R32F;
    case format::c2:
      return GL_RG32F;
    case format::c3:
      return GL_RGB32F;
    case format::c4:
      break;
  }
  return GL_RGBA32F;
}

[[nodiscard]] auto
to_gl_format(const format fmt) -> GLenum
{
  switch (fmt) {
    case format::c1:
      return GL_RED;
    case format::c2:
      return GL_RG;
    case format::c3:
      return GL_RGB;
    case format::c4:
      break;
  }
  return GL_RGBA;
}

class texture final : public landbrush::texture
{
public:
  texture(const size& s, const format fmt)
    : size_(s)
    , format_(fmt)
  {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 to_gl_internal_format(fmt),
                 s.width,
                 s.height,
                 0,
                 to_gl_format(fmt),
                 GL_FLOAT,
                 nullptr);

    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  ~texture()
  {
    glDeleteFramebuffers(1, &framebuffer_);

    glDeleteTextures(1, &texture_);
  }

  [[nodiscard]] auto get_format() const -> format override { return format_; }

  [[nodiscard]] auto get_size() const -> size override { return size_; }

  void read(float* data) const override
  {
    //
  }

  void write(const float* data) override
  {
    //
  }

private:
  GLuint framebuffer_{};

  GLuint texture_{};

  size size_{ 0, 0 };

  format format_{};
};

class opengl_backend final : public landbrush::opengl_backend
{
public:
  explicit opengl_backend(loader ldr) { gladLoadGLES2(reinterpret_cast<GLADloadfunc>(ldr)); }

  auto create_texture(const uint16_t w, const uint16_t h, const format fmt)
    -> std::unique_ptr<landbrush::texture> override
  {
    return std::make_unique<texture>(size{ w, h }, fmt);
  }

  auto get_shader(const char* name) -> shader* override
  {
    // TODO
    return nullptr;
  }

private:
};

} // namespace opengl

} // namespace

auto
opengl_backend::create(loader ldr) -> std::shared_ptr<opengl_backend>
{
  return std::make_shared<opengl::opengl_backend>(ldr);
}

} // namespace landbrush
