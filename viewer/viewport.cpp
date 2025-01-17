#include "viewport.h"

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
