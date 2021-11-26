#include "Terrain.h"

#include <vector>

#include <cassert>
#include <cstring>

Terrain::Terrain(int w, int h)
  : m_width(w)
  , m_height(h)
{
  const int vertexCount = w * h * 6;

  std::vector<glm::vec2> vertices(vertexCount);

  for (int y = 0; y < h; y++) {

    // clang-format off
    const glm::vec2 base[6] {
      glm::vec2(0, 0),
      glm::vec2(0, 1),
      glm::vec2(1, 0),
      glm::vec2(1, 0),
      glm::vec2(0, 1),
      glm::vec2(1, 1)
    };
    // clang-format on

    for (int x = 0; x < w; x++) {

      const int i = ((y * w) + x) * 6;

      for (int j = 0; j < 6; j++) {
        vertices[i + j] = (base[j] + glm::vec2(x, y)) / glm::vec2(w, h);
      }
    }
  }

  m_vertexBuffer.bind();
  m_vertexBuffer.allocate(vertexCount, GL_STATIC_DRAW);
  m_vertexBuffer.write(0, &vertices[0], vertexCount);
  m_vertexBuffer.unbind();

  m_heightMap.bind();
  m_heightMap.resize(w, h, GL_R32F, GL_RED, GL_FLOAT);
  m_heightMap.unbind();

  m_waterMap.bind();
  m_waterMap.resize(w, h, GL_R32F, GL_RED, GL_FLOAT);
  m_waterMap.unbind();

  std::vector<float> initData(w * h, 0.0f);

  setHeightMap(&initData[0], w, h);
}

void
Terrain::setHeightMap(const float* height, int w, int h)
{
  assert(w == m_width);
  assert(h == m_height);

  m_heightMapBuffer.resize(w * h);

  std::memcpy(m_heightMapBuffer.data(), height, w * h * sizeof(float));

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_heightMap.bind();
  m_heightMap.write(level, x, y, w, h, height);
  m_heightMap.unbind();
}

void
Terrain::setWaterMap(const float* water, int w, int h)
{
  assert(w == m_width);
  assert(h == m_height);

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_waterMap.bind();
  m_waterMap.write(level, x, y, w, h, water);
  m_waterMap.unbind();
}

void
Terrain::draw()
{
  m_vertexBuffer.bind();

  m_heightMap.bind();

  glActiveTexture(GL_TEXTURE1);
  m_waterMap.bind();

  glDrawArrays(GL_TRIANGLES, 0, width() * height() * 6);

  m_waterMap.unbind();

  glActiveTexture(GL_TEXTURE0);
  m_heightMap.unbind();

  m_vertexBuffer.unbind();
}
