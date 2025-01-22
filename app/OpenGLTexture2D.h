#pragma once

#include <glad/glad.h>

#include <glm/fwd.hpp>

class OpenGLTexture2D final
{
public:
  OpenGLTexture2D();

  OpenGLTexture2D(OpenGLTexture2D&& other);

  ~OpenGLTexture2D();

  OpenGLTexture2D(const OpenGLTexture2D&) = delete;

  GLint getIntegerParameter(GLenum parameterName) const
  {
    assert(m_boundFlag);
    GLint value = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, parameterName, &value);
    return value;
  }

  /// Opens an image file and loads the data onto the texture.
  ///
  /// @note The texture must be bound before calling this function.
  ///
  /// @param path The path of the image to open.
  ///
  /// @param flipVertically Whether or not the image should be flipped when loaded.
  ///
  /// @return True on success, false on failure.
  bool openFile(const char* path, bool flipVertically = true);

  GLuint id() noexcept { return m_textureID; }

  bool isBound() const noexcept { return m_boundFlag; }

  void bind();

  void unbind();

  void setMinMagFilters(GLenum minFilter, GLenum magFilter);

  void resize(GLint w, GLint h, GLenum internalFormat, GLenum format, GLenum type);

  void write(GLint level, GLint x, GLint y, GLint w, GLint h, GLenum format, GLenum type, const void* pixels);

  void write(GLint level, GLint x, GLint y, GLint w, GLint h, const float* r);

  void write(GLint level, GLint x, GLint y, GLint w, GLint h, const glm::vec3* rgb);

  void write(GLint level, GLint x, GLint y, GLint w, GLint h, const glm::vec4* rgba);

  bool isCreated() const noexcept { return m_textureID > 0; }

private:
  GLuint m_textureID = 0;

  bool m_boundFlag = false;
};
