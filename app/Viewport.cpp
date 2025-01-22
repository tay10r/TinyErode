#include "Viewport.h"

namespace {

class ViewportImpl final : public Viewport
{
public:
  [[nodiscard]] auto Setup() -> bool override
  {
    //
    return true;
  }

  void Teardown() override
  {
    //
  }

  void Step() override
  {
    //
  }
};

} // namespace

auto
Viewport::Create() -> std::unique_ptr<Viewport>
{
  return std::make_unique<ViewportImpl>();
}
