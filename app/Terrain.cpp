#include "Terrain.h"

#include <vector>

#include <cassert>
#include <cstring>

Terrain::Terrain(const int w, const int h)
  : m_width(w)
    , m_height(h)
{
  const int vertexCount = w * h * 6;

  std::vector<glm::vec2> vertices(vertexCount);

  for (int y = 0; y < h; y++) {

    // clang-format off
    const glm::vec2 base[6]{
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
  m_vertexBuffer.write(0, vertices.data(), vertexCount);
  m_vertexBuffer.unbind();

  m_glModel.rockTexture.bind();
  m_glModel.rockTexture.resize(w, h, GL_R32F, GL_RED, GL_FLOAT);
  m_glModel.rockTexture.unbind();

  m_glModel.soilTexture.bind();
  m_glModel.soilTexture.resize(w, h, GL_R32F, GL_RED, GL_FLOAT);
  m_glModel.soilTexture.unbind();

  m_glModel.waterTexture.bind();
  m_glModel.waterTexture.resize(w, h, GL_R32F, GL_RED, GL_FLOAT);
  m_glModel.waterTexture.unbind();

  m_glModel.splatTexture.bind();
  m_glModel.splatTexture.resize(w, h, GL_RGBA32F, GL_RGBA, GL_FLOAT);
  m_glModel.splatTexture.unbind();

  const std::vector<float> initData(w * h, 0.0f);

  setSoilHeight(initData.data(), w, h);
}

void
Terrain::setSoilHeight(const float* height, const int w, const int h)
{
  assert(w == m_width);
  assert(h == m_height);

  auto* s = getSnapshot();

  s->soilHeight->resize(w * h);

  std::memcpy(s->soilHeight->data(), height, w * h * sizeof(float));

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_glModel.soilTexture.bind();
  m_glModel.soilTexture.write(level, x, y, w, h, height);
  m_glModel.soilTexture.unbind();
}

Terrain::Snapshot*
Terrain::createEdit()
{
  m_snapshots.resize(m_snapshotIndex + 1);

  m_snapshots.emplace_back(m_snapshots.at(m_snapshotIndex));

  ++m_snapshotIndex;

  return &m_snapshots.back();
}

Terrain::Snapshot*
Terrain::getSnapshot()
{
  return &m_snapshots.at(m_snapshotIndex);
}

const Terrain::Snapshot*
Terrain::getSnapshot() const
{
  return &m_snapshots.at(m_snapshotIndex);
}

void
Terrain::setWaterHeight(const float* water, int w, int h)
{
  assert(w == m_width);
  assert(h == m_height);

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_glModel.waterTexture.bind();
  m_glModel.waterTexture.write(level, x, y, w, h, water);
  m_glModel.waterTexture.unbind();
}

void
Terrain::draw()
{
  m_vertexBuffer.bind();

  glActiveTexture(GL_TEXTURE0);
  m_glModel.rockTexture.bind();

  glActiveTexture(GL_TEXTURE1);
  m_glModel.soilTexture.bind();

  glActiveTexture(GL_TEXTURE2);
  m_glModel.waterTexture.bind();

  glDrawArrays(GL_TRIANGLES, 0, width() * height() * 6);

  // glActiveTexture(GL_TEXTURE2);
  m_glModel.waterTexture.unbind();

  glActiveTexture(GL_TEXTURE1);
  m_glModel.soilTexture.unbind();

  glActiveTexture(GL_TEXTURE0);
  m_glModel.rockTexture.unbind();

  m_vertexBuffer.unbind();
}
