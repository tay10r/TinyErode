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

  void setWaterMap(const float* water, int w, int h);

  const float* getHeightMapBuffer() const noexcept { return m_heightMapBuffer.data(); }

  void draw();

  float totalMetersX() const noexcept { return m_totalMetersX; }

  float totalMetersY() const noexcept { return m_totalMetersY; }

  void setTotalMetersX(float meters) noexcept { m_totalMetersX = meters; }

  void setTotalMetersY(float meters) noexcept { m_totalMetersY = meters; }

private:
  OpenGLVertexBuffer<glm::vec2> m_vertexBuffer;

  OpenGLTexture2D m_heightMap;

  OpenGLTexture2D m_waterMap;

  std::vector<float> m_heightMapBuffer;

  int m_width = 0;

  int m_height = 0;

  float m_totalMetersX = 1000.0f;

  float m_totalMetersY = 1000.0f;
};
