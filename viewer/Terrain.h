#pragma once

#include "OpenGLShaderProgram.h"
#include "OpenGLTexture2D.h"
#include "OpenGLVertexBuffer.h"

#include <glm/vec3.hpp>

class Terrain final
{
public:
  Terrain(int w, int h);

  int width() const noexcept { return m_width; }

  int height() const noexcept { return m_height; }

  void setHeightMap(const float* height, int w, int h);

  void draw();

private:
  OpenGLVertexBuffer<glm::vec2> m_vertexBuffer;

  OpenGLTexture2D m_heightMap;

  OpenGLTexture2D m_waterMap;

  int m_width = 0;

  int m_height = 0;
};
