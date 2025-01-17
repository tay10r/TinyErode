#include "main_menu_bar.h"

#include <imgui.h>

namespace {

[[nodiscard]] auto
ReadFlag(const UiFlags* flags, const UiFlags flag) -> bool
{
  return (static_cast<uint32_t>(*flags) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
}

void
SetFlag(UiFlags* flags, const UiFlags flag, bool set)
{
  if (set) {
    *flags = static_cast<UiFlags>(static_cast<uint32_t>(*flags) | static_cast<uint32_t>(flag));
  } else {
    *flags = static_cast<UiFlags>(static_cast<uint32_t>(*flags) & (0xffff'ffff ^ static_cast<uint32_t>(flag)));
  }
}

} // namespace

MainMenuBar::MainMenuBar(UiFlags* flags)
  : flags_(flags)
{
}

void
MainMenuBar::Step()
{
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("View")) {

    FlagCheckbox("Help", UiFlags::kHelp);

    FlagCheckbox("Viewport", UiFlags::kViewport);

    FlagCheckbox("Layers", UiFlags::kLayers);

    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void
MainMenuBar::FlagCheckbox(const char* label, const UiFlags flag)
{
  bool state = ReadFlag(flags_, flag);

  ImGui::Checkbox(label, &state);

  SetFlag(flags_, flag, state);
}
