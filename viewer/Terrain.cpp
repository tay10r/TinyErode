#include "Terrain.h"

#include <vector>

#include <cassert>

namespace {

const char* gTerrainVert = R"(
#version 300 es

layout(location = 0) in vec2 position;

uniform highp mat4 mvp;

void
main()
{
  gl_Position = mvp * vec4(position.x, 0, position.y, 1.0);
}
)";

const char* gTerrainFrag = R"(
#version 300 es

out lowp vec4 outColor;

void
main()
{
  outColor = vec4(1, 1, 1, 1);
}
)";

} // namespace

Terrain::Terrain(int w, int h)
  : m_width(w)
  , m_height(h)
{
  std::vector<glm::vec2> vertices((w + 1) * (h + 1) * 6);

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

      const int i = ((y * (w + 1)) + x) * 6;

      for (int j = 0; j < 6; j++) {
        vertices[i + j] = (base[j] + glm::vec2(x, y)) / glm::vec2(w, h);
      }
    }
  }

  m_vertexBuffer.bind();
  m_vertexBuffer.allocate(6 * (w + 1) * (h + 1), GL_STATIC_DRAW);
  m_vertexBuffer.write(0, &vertices[0], (w + 1) * (h + 1) * 6);
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

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_heightMap.bind();
  m_heightMap.write(level, x, y, w, h, height);
  m_heightMap.unbind();
}

void
Terrain::render(const glm::mat4& mvp)
{
  m_terrainProgram.bind();

  m_terrainProgram.setUniformValue("mvp", mvp);

  m_vertexBuffer.bind();

  glDrawArrays(GL_TRIANGLES, 0, (width() + 1) * (height() + 1));

  m_vertexBuffer.unbind();

  m_terrainProgram.unbind();
}

bool
Terrain::compileShaders(std::ostream& errStream)
{
  if (!m_terrainProgram.makeSimpleProgram(gTerrainVert, gTerrainFrag, errStream))
    return false;

  return true;
}
