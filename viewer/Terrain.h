#pragma once

#include "OpenGLShaderProgram.h"
#include "OpenGLTexture2D.h"
#include "OpenGLVertexBuffer.h"

#include <glm/vec3.hpp>

#include <vector>
#include <memory>
#include <future>

/// This is the model for a terrain, at least as it pertains to this program.
/// It includes information used for rendering, such as the OpenGL textures for the height and water levels.
class Terrain final
{
public:
  /// Contains the data in a terrain after a user modification.
  struct Snapshot final
  {
    /// The type for a floating point buffer.
    using FloatBuffer = std::vector<float>;

    Snapshot()
      : Snapshot(2, 2)
    {
    }

    Snapshot(const int w_param, const int h_param)
      : w(w_param)
        , h(h_param)
    {
    }

    /// The width of each texture, in terms of texels.
    const int w;

    /// The height of each texture, in terms of texels.
    const int h;

    /// The height of rock layer.
    std::shared_ptr<FloatBuffer> rockHeight{ new FloatBuffer(static_cast<FloatBuffer::size_type>(w * h)) };

    /// The height of soil layer.
    std::shared_ptr<FloatBuffer> soilHeight{ new FloatBuffer(static_cast<FloatBuffer::size_type>(w * h)) };

    /// The height of water layer.
    std::shared_ptr<FloatBuffer> waterHeight{ new FloatBuffer(static_cast<FloatBuffer::size_type>(w * h)) };

    /// The alpha channels of each of the 4 colors.
    std::shared_ptr<FloatBuffer> splat{ new FloatBuffer(static_cast<FloatBuffer::size_type>(w * h)) };

    /// The physical size of the terrain, in each dimension.
    float physicalSize[2]{ static_cast<float>(w) - 1.0f, static_cast<float>(h) - 1.0f };
  };

  /// Contains the view model of the terrain.
  struct GlModel final
  {
    /// The rock height texture.
    OpenGLTexture2D rockTexture;

    /// The soil height texture.
    OpenGLTexture2D soilTexture;

    /// The water height texture.
    OpenGLTexture2D waterTexture;

    /// The alpha channel of each terrain property (rock, soil, water, reserved_for_future_use)
    OpenGLTexture2D splatTexture;
  };

  /// When work is being done on the terrain, it run is done as a "job".
  ///
  /// A job will build a new snapshot of the terrain in the background, and optionally update the GL model if a preview is requested.
  class Job
  {
  public:
    Job() = default;

    Job(const Job&) = default;

    Job(Job&&) = default;

    Job& operator=(const Job&) = default;

    Job& operator=(Job&&) = default;

    virtual ~Job() = default;

    /// Checks on the status of a job.
    ///
    /// @param snapshot The snapshot that the job is working on.
    ///
    /// @param glModel The GL model to update if previews are requested.
    ///
    /// @return True if the job has completed, false if it is still active.
    virtual bool poll(Snapshot& snapshot, GlModel& glModel) = 0;

    /// Cancels the job.
    virtual void cancel() = 0;
  };

  /// Constructs a new terrain instance.
  ///
  /// @param w The width of the terrain, in terms of texels.
  ///
  /// @param h The height of the terrain, in terms of texels.
  Terrain(int w, int h);

  /// Gets the width of the terrain, in terms of pixels.
  ///
  /// @return The width of the terrain, in pixels.
  int width() const noexcept { return m_width; }

  /// Gets the height of the terrain, in terms of pixels.
  ///
  /// @return The height of the terrain, in terms of pixels.
  int height() const noexcept { return m_height; }

  /// Gets a pointer to the buffer containing the soil heights.
  ///
  /// @return A pointer containing the height of each soil cell.
  const float* getSoilBuffer() const noexcept { return getSnapshot()->soilHeight->data(); }

  void draw();

  float totalMetersX() const noexcept { return m_totalMetersX; }

  float totalMetersY() const noexcept { return m_totalMetersY; }

  void setTotalMetersX(const float meters) noexcept { m_totalMetersX = meters; }

  void setTotalMetersY(const float meters) noexcept { m_totalMetersY = meters; }

  /// Gets a pointer to the OpenGL view model.
  ///
  /// @return A pointer to the OpenGL view model.
  GlModel* glModel() { return &m_glModel; }

  /// Gets a pointer to the current snapshot of the terrain.
  ///
  /// @return A pointer to the current snapshot of the terrain.
  const Snapshot* getSnapshot() const;

  /// Creates a new entry within the edit history of the terrain.
  ///
  /// @return A pointer to the new snapshot of the terrain.
  Snapshot* createEdit();

  /// Indicates whether or not the terrain is locked.
  /// The terrain is locked when a job is running that is in the middle of modifying the terrain.
  ///
  /// @return True if the terrain is locked, false otherwise.
  bool isLocked() const { return m_currentJob.get() != nullptr; }

  /// Sets the texture data of the water levels in the terrain.
  ///
  /// @param water The buffer containing the heights of each water cell.
  ///
  /// @param w The width of the water texture.
  ///
  /// @param h The height of the water texture.
  void setWaterHeight(const float* water, int w, int h);

  /// Sets the height of each cell in the soil texture.
  ///
  /// @param height The buffer containing the heights of each cell.
  ///
  /// @param w The width of the soil texture, in terms of pixels.
  ///
  /// @param h The height of the soil texture, in terms of pixels.
  void setSoilHeight(const float* height, int w, int h);

private:
  //===================//
  // Private Functions //
  //===================//

  /// Gets a read+write pointer to the current snapshot.
  ///
  /// @note This should only be used during initialization.
  ///       Modifications to the terrain should normally generate a new snapshot.
  ///
  /// @return A pointer to the current snapshot.
  Snapshot* getSnapshot();

  //==================//
  // Member Variables //
  //==================//

  OpenGLVertexBuffer<glm::vec2> m_vertexBuffer;

  /// The view model for the terrain.
  GlModel m_glModel;

  int m_width = 0;

  int m_height = 0;

  float m_totalMetersX = 255.0f;

  float m_totalMetersY = 255.0f;

  /// The collection of snapshots for each edit.
  std::vector<Snapshot> m_snapshots{ Snapshot(m_width, m_height) };

  /// The index of the current snapshot.
  std::vector<Snapshot>::size_type m_snapshotIndex{ 0 };

  /// A pointer to a job currently running on the terrain (null if there is no job).
  std::unique_ptr<Job> m_currentJob;
};
