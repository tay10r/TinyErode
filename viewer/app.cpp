#include "app.h"

#include "help.h"
#include "layer_view.h"
#include "main_menu_bar.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace {

class AppImpl final : public App
{
public:
  explicit AppImpl(GLFWwindow* window)
    : window_(window)
  {
  }

  void Setup() override
  {
    ImGuiSettingsHandler settings_handler;
    settings_handler.TypeName = "TinyErode";
    settings_handler.TypeHash = ImHashStr("TinyErode");
    auto& io = ImGui::GetIO();
    io.IniFilename = "tinyerode.ini";
    //
  }

  void Teardown() override
  {
    //
  }

  void Step() override
  {
    main_menu_bar_.Step();

    if (IsSet(ui_flags_, UiFlags::kHelp)) {
      if (BeginWidget("Help", UiFlags::kHelp)) {
        help_.Step();
      }
      EndWidget();
    }

    if (IsSet(ui_flags_, UiFlags::kViewport)) {
    }
  }

  [[nodiscard]] auto GetStatus() -> Status override { return status_; }

protected:
  [[nodiscard]] auto BeginWidget(const char* label, const UiFlags flag) -> bool
  {
    ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(512, 512), ImGuiCond_FirstUseEver);
    auto is_open{ true };
    const auto return_value{ ImGui::Begin(label, &is_open) };
    if (!is_open) {
      ui_flags_ = Unset(ui_flags_, flag);
    }
    return return_value;
  }

  static void EndWidget() { ImGui::End(); }

private:
  GLFWwindow* window_{};

  App::Status status_{ Status::kRunning };

  UiFlags ui_flags_{ UiFlags::kHelp };

  MainMenuBar main_menu_bar_{ &ui_flags_ };

  Help help_{};
};

} // namespace

auto
App::Create(GLFWwindow* window) -> std::unique_ptr<App>
{
  return std::make_unique<AppImpl>(window);
}
