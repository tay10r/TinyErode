#include "NoiseFilter.h"

#include <imgui.h>

#include "FastNoiseLite.h"
#include "Terrain.h"

#include <vector>

namespace {

const char*
toString(FastNoiseLite::NoiseType noiseType)
{
  switch (noiseType) {
    case FastNoiseLite::NoiseType_OpenSimplex2:
      return "OpenSimplex2";
    case FastNoiseLite::NoiseType_OpenSimplex2S:
      return "OpenSimplex2S";
    case FastNoiseLite::NoiseType_Cellular:
      return "Cellular";
    case FastNoiseLite::NoiseType_Perlin:
      return "Perlin";
    case FastNoiseLite::NoiseType_ValueCubic:
      return "ValueCubic";
    case FastNoiseLite::NoiseType_Value:
      return "Value";
  }

  return "";
}

int
toIndex(FastNoiseLite::NoiseType noiseType)
{
  switch (noiseType) {
    case FastNoiseLite::NoiseType_OpenSimplex2:
      return 0;
    case FastNoiseLite::NoiseType_OpenSimplex2S:
      return 1;
    case FastNoiseLite::NoiseType_Cellular:
      return 2;
    case FastNoiseLite::NoiseType_Perlin:
      return 3;
    case FastNoiseLite::NoiseType_ValueCubic:
      return 4;
    case FastNoiseLite::NoiseType_Value:
      return 5;
  }

  return -1;
}

bool
ImGui_NoiseType(FastNoiseLite::NoiseType* noiseType, ImGuiComboFlags flags = 0)
{
  const char* current = toString(*noiseType);

  const char* items[]{
    "OpenSimplex2", "OpenSimplex2S", "Cellular", "Perlin", "ValueCubic", "Value",
  };

  int currentIndex = -1;

  for (int i = 0; i < 6; i++) {
    if (strcmp(items[i], current) == 0) {
      currentIndex = i;
      break;
    }
  }

  if (ImGui::Combo("Noise Type", &currentIndex, items, 6)) {
    switch (currentIndex) {
      case 0:
        *noiseType = FastNoiseLite::NoiseType_OpenSimplex2;
        break;
      case 1:
        *noiseType = FastNoiseLite::NoiseType_OpenSimplex2S;
        break;
      case 2:
        *noiseType = FastNoiseLite::NoiseType_Cellular;
        break;
      case 3:
        *noiseType = FastNoiseLite::NoiseType_Perlin;
        break;
      case 4:
        *noiseType = FastNoiseLite::NoiseType_ValueCubic;
        break;
      case 5:
        *noiseType = FastNoiseLite::NoiseType_Value;
        break;
    }
    return true;
  }

  return false;
}

enum class BlendMode
{
  Replace,
  Multiply,
  Add,
  Subtract
};

const char*
toString(BlendMode blendMode)
{
  switch (blendMode) {
    case BlendMode::Replace:
      return "Replace";
    case BlendMode::Multiply:
      return "Multiply";
    case BlendMode::Add:
      return "Add";
    case BlendMode::Subtract:
      return "Subtract";
  }

  return "";
}

bool
ImGui_BlendMode(BlendMode* blendMode, ImGuiComboFlags flags = 0)
{
  const char* current = toString(*blendMode);

  const char* items[]{ "Replace", "Multiply", "Add", "Subtract" };

  const int itemCount = 4;

  int currentIndex = -1;

  for (int i = 0; i < itemCount; i++) {
    if (strcmp(items[i], current) == 0) {
      currentIndex = i;
      break;
    }
  }

  if (ImGui::Combo("Blend Mode", &currentIndex, items, itemCount)) {
    switch (currentIndex) {
      case 0:
        *blendMode = BlendMode::Replace;
        break;
      case 1:
        *blendMode = BlendMode::Multiply;
        break;
      case 2:
        *blendMode = BlendMode::Add;
        break;
      case 3:
        *blendMode = BlendMode::Subtract;
        break;
    }

    return true;
  }

  return false;
}

} // namespace

class NoiseFilterImpl final
{
  friend NoiseFilter;

  int seed = 1337;

  float frequency = 0.01f;

  float scale = 1.0f;

  FastNoiseLite::NoiseType noiseType = FastNoiseLite::NoiseType_Perlin;

  BlendMode blendMode = BlendMode::Replace;

  void generateNoise(Terrain& terrain);

  template<typename BlendFunc>
  static void blend(const float* a, const float* b, float* c, int size, BlendFunc);
};

NoiseFilter::NoiseFilter()
  : m_self(new NoiseFilterImpl())
{}

NoiseFilter::~NoiseFilter()
{
  delete m_self;
}

void
NoiseFilter::renderGui(Terrain& terrain)
{
  ImGui::SliderInt("Randomize Seed", &m_self->seed, 0, 0x3fffffff);

  ImGui_NoiseType(&m_self->noiseType);

  ImGui_BlendMode(&m_self->blendMode);

  ImGui::SliderFloat("Scale", &m_self->scale, 0.1f, 100.0f);

  ImGui::SliderFloat("Frequency", &m_self->frequency, 0.0001f, 0.05f);

  if (ImGui::Button("Generate Noise"))
    m_self->generateNoise(terrain);
}

void
NoiseFilterImpl::generateNoise(Terrain& terrain)
{
  const int w = terrain.width();
  const int h = terrain.height();

  std::vector<float> noise(w * h, 0.0f);

  FastNoiseLite fastNoiseLite;

  fastNoiseLite.SetSeed(seed);

  fastNoiseLite.SetNoiseType(noiseType);

  fastNoiseLite.SetFrequency(frequency);

  for (int i = 0; i < (w * h); i++) {

    const int x = i % w;
    const int y = i / w;

    noise[i] = fastNoiseLite.GetNoise(float(x), float(y)) * scale;
  }

  if (blendMode == BlendMode::Replace) {
    terrain.setHeightMap(noise.data(), w, h);
    return;
  }

  std::vector<float> dstBuffer(w * h, 0.0f);

  const float* src = terrain.getHeightMapBuffer();
  const float* tmp = noise.data();
  float* dst = dstBuffer.data();

  switch (blendMode) {
    case BlendMode::Replace:
      /* Already done. */
      break;
    case BlendMode::Multiply:
      blend(src, tmp, dst, w * h, [](float a, float b) -> float { return a * b; });
      break;
    case BlendMode::Add:
      blend(src, tmp, dst, w * h, [](float a, float b) -> float { return a + b; });
      break;
    case BlendMode::Subtract:
      blend(src, tmp, dst, w * h, [](float a, float b) -> float { return a - b; });
      break;
  }

  terrain.setHeightMap(dst, w, h);
}

template<typename BlendFunc>
void
NoiseFilterImpl::blend(const float* a, const float* b, float* dst, int size, BlendFunc blendFunc)
{
  for (int i = 0; i < size; i++)
    dst[i] = blendFunc(a[i], b[i]);
}
