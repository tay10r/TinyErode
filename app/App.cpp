#include "App.h"

#include "ControlWidget.h"
#include "LayerView.h"
#include "SimState.h"
#include "Viewport.h"

#include <landbrush.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <stb_image.h>

namespace {

class AppImpl final : public App
{
public:
  explicit AppImpl(GLFWwindow* window)
    : m_window(window)
  {
  }

  void Setup() override
  {
    ImGuiSettingsHandler settings_handler;
    settings_handler.TypeName = "landbrush";
    settings_handler.TypeHash = ImHashStr("landbrush");
    auto& io = ImGui::GetIO();

    if (!m_viewport->Setup()) {
      m_viewport.reset();
    }

    m_layerView->Setup();

    Reset();
  }

  void Teardown() override
  {
    if (m_viewport) {
      m_viewport->Teardown();
    }

    m_layerView->Teardown();
  }

  void Step() override
  {
    if (m_simState) {
      m_simState->Step(*m_device);
      if (m_layerView) {
        m_layerView->Update(*m_simState);
      }
    }

    if (m_viewport) {
      ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(16, 16), ImGuiCond_FirstUseEver);
      if (ImGui::Begin("Viewport")) {
        m_viewport->Step();
      }
      ImGui::End();
    }

    if (m_layerView) {
      ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_FirstUseEver);
      ImGui::SetNextWindowPos(ImVec2(16, 32 + 256), ImGuiCond_FirstUseEver);
      if (ImGui::Begin("Layers")) {
        m_layerView->Step(m_config);
      }
      ImGui::End();
    }

    ImGui::SetNextWindowSize(ImVec2(256, 256), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(256 + 32, 16), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      const auto controlButton = m_controlWidget.Step(&m_config);
      HandleControlButton(controlButton);
    }
    ImGui::End();
  }

  [[nodiscard]] auto GetStatus() -> Status override { return m_status; }

protected:
  void Reset()
  {
    if (m_config.imagePath.empty()) {
      m_simState.reset(new SimState(*m_device, m_config));
      FastNoiseLite noise(m_config.seed);
      noise.SetFrequency(m_config.noiseFrequency);
      noise.SetFractalType(FastNoiseLite::FractalType_FBm);
      noise.SetFractalOctaves(8);
      m_simState->HeightNoise(noise, m_config.distancePerCell);
    } else {
      int w{};
      int h{};
      auto* data = stbi_load(m_config.imagePath.c_str(), &w, &h, nullptr, 1);
      if (data) {
        m_config.width = w;
        m_config.height = h;
        m_simState.reset(new SimState(*m_device, m_config));
        m_simState->HeightData(data);
      }
    }

    m_brushTexture = m_device->create_texture(m_config.width, m_config.height, landbrush::format::c1);

    m_layerView->Update(*m_simState);
  }

  void HandleControlButton(const ControlButton button)
  {
    switch (button) {
      case ControlButton::NONE:
        break;
      case ControlButton::RESET:
        Reset();
        break;
    }
  }

  static void OnBrush(void* selfPtr, const float x, const float y, const float r)
  {
    auto* self = static_cast<AppImpl*>(selfPtr);

    auto* brushShader = self->m_device->get_shader("brush");

    brushShader->set_float("x", x);
    brushShader->set_float("y", y);
    brushShader->set_float("radius", r);
    brushShader->set_float("distance_per_cell", self->m_config.distancePerCell);
    brushShader->set_texture("output", self->m_brushTexture.get());
    brushShader->invoke();

    auto& waterSB = self->m_simState->GetWaterTextureSB();

    auto* blendShader = self->m_device->get_shader("blend");
    blendShader->set_float("k_alpha", 1.0F);
    blendShader->set_float("k_beta", 1.0F);
    blendShader->set_texture("alpha", self->m_brushTexture.get());
    blendShader->set_texture("beta", waterSB.current().get());
    blendShader->set_texture("gamma", waterSB.next().get());
    blendShader->invoke();

    waterSB.step();
  }

private:
  GLFWwindow* m_window{};

  App::Status m_status{ Status::kRunning };

  Config m_config;

  std::shared_ptr<landbrush::backend> m_device{ landbrush::cpu_backend::create() };

  std::unique_ptr<Viewport> m_viewport{ Viewport::Create() };

  std::unique_ptr<LayerView> m_layerView{ LayerView::Create(this, OnBrush) };

  std::shared_ptr<landbrush::texture> m_brushTexture;

  std::unique_ptr<SimState> m_simState;

  ControlWidget m_controlWidget;
};

} // namespace

auto
App::Create(GLFWwindow* window) -> std::unique_ptr<App>
{
  return std::make_unique<AppImpl>(window);
}
