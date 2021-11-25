#include "Terrain.h"

#include <vector>

#include <cassert>

namespace {

const char* gTerrainVert = R"(
#version 300 es

layout(location = 0) in vec2 position;

uniform highp mat4 mvp;

uniform highp float metersPerPixel;

uniform sampler2D heightMap;

out highp vec3 vertexNormal;

highp vec3
computeNormal()
{
  // 2 | a b c
  // 1 | d x e
  // 0 | f g h
  //    ------
  //     0 1 2

  ivec2 texSize = textureSize(heightMap, 0);

  highp vec2 pixel_size = vec2(1.0 / float(texSize.x), 1.0 / float(texSize.y));

  highp vec2 uv[9];

  // a b c
  uv[0] = position + vec2(-pixel_size.x, -pixel_size.y);
  uv[1] = position + vec2(            0, -pixel_size.y);
  uv[2] = position + vec2( pixel_size.x, -pixel_size.y);

  // d x e
  uv[3] = position + vec2(-pixel_size.x, 0);
  uv[4] = position + vec2(            0, 0);
  uv[5] = position + vec2( pixel_size.x, 0);

  // f g h
  uv[6] = position + vec2(-pixel_size.x, pixel_size.y);
  uv[7] = position + vec2(            0, pixel_size.y);
  uv[8] = position + vec2( pixel_size.x, pixel_size.y);

  highp vec3 p[9];

  for (int i = 0; i < 9; i++) {

    highp float y = texture(heightMap, uv[i]).r;

    highp vec2 ndc = (uv[i] * 2.0) - 1.0;

    p[i] = vec3(ndc.x, texture(heightMap, uv[i]).r, ndc.y);
  }

  highp vec3 center = p[4];

  highp vec3 edges[9];

  // a b c
  edges[0] = p[0] - center;
  edges[1] = p[1] - center;
  edges[2] = p[2] - center;

  // e
  edges[3] = p[5] - center;

  // h g f
  edges[4] = p[8] - center;
  edges[5] = p[7] - center;
  edges[6] = p[6] - center;

  // d
  edges[7] = p[3] - center;

  // a
  edges[8] = edges[0];

  // TODO : reject angles above a certain threshold angle.

  int accepted_normal_count = 0;

  highp vec3 normal_sum = vec3(0, 0, 0);

  for (int i = 0; i < 8; i++) {

    normal_sum += normalize(cross(edges[i], edges[i + 1]));

    /* TODO : Make an angle threshold for user. */

    accepted_normal_count++;
  }

  return -normalize(normal_sum / float(accepted_normal_count));
}

void
main()
{
  highp vec2 xy = ((position * 2.0) - 1.0) * metersPerPixel;

  vertexNormal = computeNormal();

  gl_Position = mvp * vec4(xy.x, texture(heightMap, position).r, xy.y, 1.0);
}
)";

const char* gTerrainFrag = R"(
#version 300 es

in highp vec3 vertexNormal;

out lowp vec4 outColor;

uniform highp vec3 lightDir;

void
main()
{
  highp float light = (dot(normalize(lightDir), vertexNormal) + 1.0) * 0.5;

  outColor = vec4(vec3(0.8, 0.8, 0.8) * light, 1);
}
)";

} // namespace

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

  const GLint level = 0;
  const GLint x = 0;
  const GLint y = 0;

  m_heightMap.bind();
  m_heightMap.write(level, x, y, w, h, height);
  m_heightMap.unbind();
}

void
Terrain::render(const glm::mat4& mvp, float metersPerPixel, const glm::vec3& lightDir)
{
  m_terrainProgram.bind();

  m_terrainProgram.setUniformValue("mvp", mvp);

  m_terrainProgram.setUniformValue("metersPerPixel", metersPerPixel);

  m_terrainProgram.setUniformValue("lightDir", lightDir);

  m_vertexBuffer.bind();

  m_heightMap.bind();

  glDrawArrays(GL_TRIANGLES, 0, width() * height() * 6);

  m_heightMap.unbind();

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
