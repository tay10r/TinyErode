#pragma once

class NoiseFilterImpl;
class Terrain;

/// This filter is used for applying various types of noise filters.
class NoiseFilter final
{
public:
  /// Constructs a new instance of the noise filter.
  NoiseFilter();

  NoiseFilter(const NoiseFilter&) = delete;

  NoiseFilter(NoiseFilter&&) = delete;

  NoiseFilter& operator=(const NoiseFilter&) = delete;

  NoiseFilter& operator=(NoiseFilter&&) = delete;

  ~NoiseFilter();

  /// Renders the user interface associated with the noise filter.
  ///
  /// @param terrain The terrain instance that the filter would be applied to.
  void renderGui(Terrain& terrain);

private:
  /// A pointer to private implementation data.
  NoiseFilterImpl* m_self;
};
