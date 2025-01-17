#include "layer_view.h"

namespace {

class LayerViewImpl final : public LayerView
{
public:
  void Setup() override
  {
    //
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
LayerView::Create() -> std::unique_ptr<LayerView>
{
  return std::make_unique<LayerViewImpl>();
}
