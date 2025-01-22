#include "landbrush_opengl.h"

namespace {

#include "landbrush_opengl_internal.h"

} // namespace

namespace landbrush {

namespace {

namespace opengl {

class opengl_backend final : public landbrush::opengl_backend
{
public:
  explicit opengl_backend(loader ldr) { gladLoadGLES2(reinterpret_cast<GLADloadfunc>(ldr)); }

  auto create_texture(uint16_t w, uint16_t h, format fmt) -> std::shared_ptr<texture> override
  {
    // TODO
    return nullptr;
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
