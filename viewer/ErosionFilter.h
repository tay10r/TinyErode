#pragma once

#include <GLFW/glfw3.h>

class ErosionFilterImpl;
class Terrain;

class ErosionFilter final
{
public:
  ~ErosionFilter();

  void renderGui(const Terrain&);

  void erode(Terrain&);

  bool inErodeState();

private:
  ErosionFilterImpl& getSelf();

private:
  ErosionFilterImpl* m_self = nullptr;
};
