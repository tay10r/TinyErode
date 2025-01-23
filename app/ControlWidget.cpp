#include "ControlWidget.h"

#include <imgui.h>
#include <imgui_stdlib.h>

auto
ControlWidget::Step(Config* config) -> ControlButton
{
  auto button{ ControlButton::NONE };

  ImGui::SeparatorText("Initial Conditions");

  ImGui::Separator();

  ImGui::InputText("Image Path", &config->imagePath);

  const auto hasImage = !config->imagePath.empty();

  ImGui::BeginDisabled(!hasImage);

  ImGui::Checkbox("16-bit Image", &config->image16Bit);

  ImGui::EndDisabled();

  ImGui::BeginDisabled(hasImage);

  ImGui::SliderInt("Noise Seed", &config->seed, 0, 1000);

  ImGui::SliderFloat("Noise Frequency", &config->noiseFrequency, 0.01F, 2.0F);

  ImGui::DragInt("Texture Width", &config->width, 4, 4, 4096);

  ImGui::DragInt("Texture Height", &config->height, 4, 4, 4096);

  ImGui::EndDisabled();

  ImGui::SliderFloat("Rock Height [m]", &config->rock_height, 1.0F, 1000.0F);

  ImGui::SliderFloat("Initial Soil Height [m]", &config->initial_soil_height, 0.0F, 100.0F);

  if (ImGui::Button("Reset")) {
    button = ControlButton::RESET;
  }

  ImGui::SeparatorText("Simulation");

  ImGui::Separator();

  ImGui::SliderInt("Speedup", &config->speedup, 1, 500);

  ImGui::SliderFloat("Time Delta [s]", &config->timeDelta, 1.0e-5F, 1.0e-2F, "%.5f");

  ImGui::SliderFloat("Gravity [m/s^2]", &config->gravity, 0.0F, 100.0F);

  ImGui::SliderFloat("Distance per Cell [m]", &config->distancePerCell, 0.0F, 100.0F);

  ImGui::SliderFloat("Pipe Radius [m]", &config->pipeRadius, 0.0F, 10.0F);

  ImGui::SliderFloat("Carry Capacity", &config->kc, 0, 1);

  ImGui::SliderFloat("Deposition", &config->kd, 0, 1);

  ImGui::SliderFloat("Erosion", &config->ke, 0, 1);

  return button;
}
