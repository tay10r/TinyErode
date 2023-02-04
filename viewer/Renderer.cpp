#include "Renderer.h"

#include "OpenGLShaderProgram.h"
#include "Terrain.h"

namespace {

const char* gTerrainVert = R"(
#version 300 es

layout(location = 0) in vec2 position;

uniform highp mat4 mvp;

uniform highp float totalMetersX;

uniform highp float totalMetersY;

uniform sampler2D rockHeight;

uniform sampler2D soilHeight;

out highp vec3 vertexNormal;

highp vec3
computeNormal()
{
  // 2 | a b c
  // 1 | d x e
  // 0 | f g h
  //    ------
  //     0 1 2

  ivec2 texSize = textureSize(rockHeight, 0);

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

    highp float y = texture(rockHeight, uv[i]).r + texture(soilHeight, uv[i]).r;

    highp vec2 ndc = (uv[i] * 2.0) - 1.0;

    p[i] = vec3(ndc.x * totalMetersX * 0.5, y, ndc.y * totalMetersY * 0.5);
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
  highp vec2 xy = ((position * 2.0) - 1.0) * (vec2(totalMetersX, totalMetersY) * 0.5);

  vertexNormal = computeNormal();

  gl_Position = mvp * vec4(xy.x, texture(rockHeight, position).r + texture(soilHeight, position).r, xy.y, 1.0);
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

const char* gWaterVert = R"(
#version 300 es

layout(location = 0) in vec2 position;

uniform highp mat4 mvp;

uniform highp float totalMetersX;

uniform highp float totalMetersY;

uniform sampler2D rockHeight;

uniform sampler2D soilHeight;

uniform sampler2D waterHeight;

out highp float waterLevel;

void
main()
{
  highp float waterOffset = texture(rockHeight, position).r + texture(soilHeight, position).r;

  highp float waterHeightTexel = texture(waterHeight, position).r;

  highp vec2 xy = ((position * 2.0) - 1.0) * (vec2(totalMetersX, totalMetersY) * 0.5);

  highp float level = waterOffset + waterHeightTexel;

  waterLevel = waterHeightTexel;

  gl_Position = mvp * vec4(xy.x, level, xy.y, 1.0);
}
)";

const char* gWaterFrag = R"(
#version 300 es

in highp float waterLevel;

out lowp vec4 outColor;

void
main()
{
  lowp float alpha = clamp(pow(waterLevel, 0.2), 0.0, 1.0);

  outColor = vec4(0.0, 0.0, 1.0, alpha);
}
)";

} // namespace

class RendererImpl final
{
  friend Renderer;

  OpenGLShaderProgram program;

  OpenGLShaderProgram waterProgram;
};

Renderer::~Renderer()
{
  delete m_self;
}

bool
Renderer::compileShaders(std::ostream& errStream)
{
  if (!getSelf().program.makeSimpleProgram(gTerrainVert, gTerrainFrag, errStream))
    return false;

  if (!getSelf().waterProgram.makeSimpleProgram(gWaterVert, gWaterFrag, errStream))
    return false;

  setMVP(glm::mat4(1.0f));
  setLightDir(glm::normalize(glm::vec3(1, 1, 0)));
  setTotalMetersX(100.0f);
  setTotalMetersY(100.0f);

  return true;
}

void
Renderer::render(Terrain& terrain)
{
  RendererImpl& self = getSelf();

  self.program.bind();

  glUniform1i(self.program.getUniformLocation("rockHeight"), 0);
  glUniform1i(self.program.getUniformLocation("soilHeight"), 1);

  terrain.draw();

  self.program.unbind();
}

void
Renderer::renderWater(Terrain& terrain)
{
  RendererImpl& self = getSelf();

  self.waterProgram.bind();

  glUniform1i(self.waterProgram.getUniformLocation("rockHeight"), 0);
  glUniform1i(self.waterProgram.getUniformLocation("soilHeight"), 1);
  glUniform1i(self.waterProgram.getUniformLocation("waterHeight"), 2);

  terrain.draw();

  self.waterProgram.unbind();
}

void
Renderer::setMVP(const glm::mat4& mvp)
{
  getSelf().program.bind();
  getSelf().program.setUniformValue("mvp", mvp);
  getSelf().program.unbind();

  getSelf().waterProgram.bind();
  getSelf().waterProgram.setUniformValue("mvp", mvp);
  getSelf().waterProgram.unbind();
}

void
Renderer::setLightDir(const glm::vec3& lightDir)
{
  getSelf().program.bind();
  getSelf().program.setUniformValue("lightDir", lightDir);
  getSelf().program.unbind();
}

void
Renderer::setTotalMetersX(float meters)
{
  getSelf().program.bind();
  getSelf().program.setUniformValue("totalMetersX", meters);
  getSelf().program.unbind();

  getSelf().waterProgram.bind();
  getSelf().waterProgram.setUniformValue("totalMetersX", meters);
  getSelf().waterProgram.unbind();
}

void
Renderer::setTotalMetersY(float meters)
{
  getSelf().program.bind();
  getSelf().program.setUniformValue("totalMetersY", meters);
  getSelf().program.unbind();

  getSelf().waterProgram.bind();
  getSelf().waterProgram.setUniformValue("totalMetersY", meters);
  getSelf().waterProgram.unbind();
}

RendererImpl&
Renderer::getSelf()
{
  if (!m_self)
    m_self = new RendererImpl();

  return *m_self;
}
