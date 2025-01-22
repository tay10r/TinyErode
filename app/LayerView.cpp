#include "LayerView.h"

#include "Config.h"
#include "SimState.h"

#include <landbrush.h>

#include <implot.h>

#include <GLES3/gl3.h>

#include <vector>

#include <cmath>

namespace {

constexpr int numBrushPoints{ 32 };

enum class Layer
{
  ROCK,
  WATER,
  ROCK_WATER
};

auto
ToString(const Layer layer) -> const char*
{
  switch (layer) {
    case Layer::ROCK:
      return "Rock";
    case Layer::WATER:
      return "Water";
    case Layer::ROCK_WATER:
      return "Rock+Water";
  }
  return "";
}

auto
ComboBox(const char* label, Layer* layer) -> bool
{
  if (ImGui::BeginCombo(label, ToString(*layer))) {
    if (ImGui::Selectable(ToString(Layer::ROCK), *layer == Layer::ROCK)) {
      *layer = Layer::ROCK;
    }
    if (ImGui::Selectable(ToString(Layer::WATER), *layer == Layer::WATER)) {
      *layer = Layer::WATER;
    }
    if (ImGui::Selectable(ToString(Layer::ROCK_WATER), *layer == Layer::ROCK_WATER)) {
      *layer = Layer::ROCK_WATER;
    }
    ImGui::EndCombo();
    return false;
  }
  return false;
}

class LayerViewImpl final : public LayerView
{
public:
  LayerViewImpl(void* brushCallbackData, BrushCallback brushCallback)
    : m_brushCallbackData(brushCallbackData)
    , m_brushCallback(brushCallback)
  {
  }

  void Setup() override
  {
    m_normBrushX.resize(numBrushPoints);
    m_normBrushY.resize(numBrushPoints);

    m_brushX.resize(numBrushPoints);
    m_brushY.resize(numBrushPoints);

    for (int i = 0; i < numBrushPoints; i++) {
      const auto angle = (static_cast<float>(i) / static_cast<float>(numBrushPoints)) * 3.1415F * 2.0f;
      m_normBrushX[i] = std::cos(angle);
      m_normBrushY[i] = std::sin(angle);
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  void Teardown() override { glDeleteTextures(1, &m_texture); }

  void Step(const Config& config) override
  {
    ComboBox("Layer", &m_layer);

    ImGui::DragFloat("Brush Radius [m]", &m_brushRadius, 1, 1000);

    if (!ImPlot::BeginPlot("##LayerView",
                           ImVec2(-1, -1),
                           ImPlotFlags_Equal | ImPlotFlags_NoFrame | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMenus)) {
      return;
    }

    ImPlot::SetupAxes("X [m]", "Y [m]");

    ImPlot::PlotImage("##Layer",
                      static_cast<ImTextureID>(m_texture),
                      ImPlotPoint(0, 0),
                      ImPlotPoint(config.width * config.distancePerCell, config.height * config.distancePerCell),
                      ImVec2(0, 1),
                      ImVec2(1, 0));

    auto brushState{ false };

    ImPlotPoint brushPoint{ 0, 0 };

    if (ImPlot::IsPlotHovered()) {
      brushPoint = ImPlot::GetPlotMousePos();
      TransformBrush(brushPoint.x, brushPoint.y, m_brushRadius);
      ImPlot::PlotLine("##Brush", m_brushX.data(), m_brushY.data(), numBrushPoints, ImPlotLineFlags_Loop);
      if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        brushState = true;
      }
    }

    Brush(brushPoint.x, brushPoint.y, /*state=*/brushState);

    ImPlot::EndPlot();
  }

  void Update(SimState& s) override
  {
    const auto& config = s.GetConfig();

    switch (m_layer) {
      case Layer::ROCK:
        UpdateTexture(s.GetRockTexture(), m_layer, config);
        break;
      case Layer::WATER:
        UpdateTexture(s.GetWaterTexture(), m_layer, config);
        break;
      case Layer::ROCK_WATER:
        RenderRockAndWater(s.GetRockTexture(), s.GetWaterTexture(), config);
        break;
    }
  }

protected:
  template<typename Scalar>
  static auto Clamp(Scalar x, Scalar min_x, Scalar max_x) -> Scalar
  {
    return std::max(std::min(x, max_x), min_x);
  }

  void RenderRockAndWater(const landbrush::texture& rockTex, landbrush::texture& waterTex, const Config& config)
  {
    const auto w = config.width;
    const auto h = config.height;

    std::vector<float> rockData(w * h);
    rockTex.read(rockData.data());

    std::vector<float> waterData(w * h);
    waterTex.read(waterData.data());

    m_rgbaData.resize(w * h * 4);

    const auto numCells = w * h;

    const auto eScale = 1.0F / config.maxElevation;

    for (auto i = 0; i < numCells; i++) {

      float rgba[4]{ 0, 0, 0, 1 };

      const auto r = Clamp(rockData[i] * eScale, 0.0F, 1.0F);
      const auto w = Clamp(waterData[i], 0.0F, 1.0F);

      rgba[0] = r;
      rgba[1] = r;
      rgba[2] = r + (w - r) * w;

      const uint8_t pixel[4]{ static_cast<uint8_t>(Clamp(static_cast<int>(rgba[0] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[1] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[2] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[3] * 255), 0, 255)) };

      m_rgbaData[i * 4 + 0] = pixel[0];
      m_rgbaData[i * 4 + 1] = pixel[1];
      m_rgbaData[i * 4 + 2] = pixel[2];
      m_rgbaData[i * 4 + 3] = pixel[3];
    }

    UpdateTexture(landbrush::size{ static_cast<uint16_t>(w), static_cast<uint16_t>(h) });
  }

  void UpdateTexture(const landbrush::texture& tex, Layer layer, const Config& config)
  {
    const auto format = tex.get_format();

    const auto numFeatures = static_cast<int>(format);

    const auto size = tex.get_size();

    const auto s = tex.get_format();

    m_textureData.resize(static_cast<size_t>(size.total()) * numFeatures);

    m_rgbaData.resize(size.width * size.height * 4);

    tex.read(m_textureData.data());

    const auto numCells = size.width * size.height;

    const auto eScale = 1.0F / config.maxElevation;

    // convert to RGBA

    for (auto i = 0; i < numCells; i++) {

      const auto* src = &m_textureData[i * numFeatures];

      float rgba[4]{ 0, 0, 0, 1 };

      switch (layer) {
        case Layer::ROCK:
          rgba[0] = src[0] * eScale;
          rgba[1] = src[0] * eScale;
          rgba[2] = src[0] * eScale;
          break;
        case Layer::WATER:
          rgba[0] = src[0];
          rgba[1] = src[0];
          rgba[2] = src[0];
          break;
        default:
          break;
      }

      const uint8_t pixel[4]{ static_cast<uint8_t>(Clamp(static_cast<int>(rgba[0] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[1] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[2] * 255), 0, 255)),
                              static_cast<uint8_t>(Clamp(static_cast<int>(rgba[3] * 255), 0, 255)) };

      m_rgbaData[i * 4 + 0] = pixel[0];
      m_rgbaData[i * 4 + 1] = pixel[1];
      m_rgbaData[i * 4 + 2] = pixel[2];
      m_rgbaData[i * 4 + 3] = pixel[3];
    }

    UpdateTexture(size);
  }

  void UpdateTexture(const landbrush::size& size)
  {
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.width, size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_rgbaData.data());

    m_layerRange[0] = size.width;
    m_layerRange[1] = size.height;
  }

  void TransformBrush(float x, float y, float r)
  {
    for (int i = 0; i < numBrushPoints; i++) {
      m_brushX[i] = x + r * m_normBrushX[i];
      m_brushY[i] = y + r * m_normBrushY[i];
    }
  }

  void Brush(const float x, const float y, const bool state)
  {
    if (m_brushCallback && state) {
      m_brushCallback(m_brushCallbackData, x, y, m_brushRadius);
    }

    // TODO : do we need this?
    m_lastBrushState = state;
  }

private:
  void* m_brushCallbackData{};

  BrushCallback m_brushCallback{};

  float m_brushRadius{ 100.0F };

  std::vector<float> m_normBrushX;

  std::vector<float> m_normBrushY;

  std::vector<float> m_brushX;

  std::vector<float> m_brushY;

  std::vector<float> m_textureData;

  std::vector<uint8_t> m_rgbaData;

  Layer m_layer{ Layer::ROCK };

  float m_layerRange[2]{ 1, 1 };

  GLuint m_texture{};

  bool m_lastBrushState{ false };
};

} // namespace

auto
LayerView::Create(void* brushCallbackData, BrushCallback brushCallback) -> std::unique_ptr<LayerView>
{
  return std::make_unique<LayerViewImpl>(brushCallbackData, brushCallback);
}
