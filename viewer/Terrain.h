#pragma once

#include "OpenGLShaderProgram.h"
#include "OpenGLTexture2D.h"
#include "OpenGLVertexBuffer.h"

#include <glm/vec3.hpp>

#include <vector>

class Terrain final
{
public:
  Terrain(int w, int h);

  int width() const noexcept { return m_width; }

  int height() const noexcept { return m_height; }

  void setHeightMap(const float* height, int w, int h);

  const float* getHeightMapBuffer() const noexcept { return m_heightMapBuffer.data(); }

  void draw();

private:
  OpenGLVertexBuffer<glm::vec2> m_vertexBuffer;

  OpenGLTexture2D m_heightMap;

  OpenGLTexture2D m_waterMap;

  std::vector<float> m_heightMapBuffer;

  int m_width = 0;

  int m_height = 0;
};
