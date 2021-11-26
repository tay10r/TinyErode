#pragma once

#include <glm/glm.hpp>

#include <iosfwd>

class RendererImpl;
class Terrain;

class Renderer final
{
public:
  ~Renderer();

  bool compileShaders(std::ostream& errStream);

  void render(Terrain& terrain);

  void renderWater(Terrain& terrain);

  void setMVP(const glm::mat4& mvp);

  void setLightDir(const glm::vec3& lightDir);

  void setMetersPerPixel(float metersPerPixel);

private:
  RendererImpl& getSelf();

private:
  RendererImpl* m_self = nullptr;
};
