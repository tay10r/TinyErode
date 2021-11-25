#pragma once

class NoiseFilterImpl;
class Terrain;

class NoiseFilter final
{
public:
  NoiseFilter();

  NoiseFilter(const NoiseFilter&) = delete;

  ~NoiseFilter();

  void renderGui(Terrain& terrain);

private:
  NoiseFilterImpl* m_self;
};
