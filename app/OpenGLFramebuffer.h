#pragma once

#include "Texture.h"

#include <GLFW/glfw3.h>

#include <glm/fwd.hpp>

#include <glad/glad.h>

#include <vector>

#ifndef NDEBUG
#include <cstdio>
#include <cstdlib>
#endif

namespace sf {

class OpenGLRenderbuffer;
class OpenGLTexture2D;

class OpenGLFramebuffer
{
public:
  /// Gets the attachments of the currently bound framebuffer.
  static std::vector<GLenum> getAttachments(GLenum framebufferTarget);

  static Texture<glm::vec4> readTextureRGBA(GLuint textureID);

  static Texture<glm::vec4> readAttachmentRGBA(GLenum colorAttachment, GLFWwindow* window);

  OpenGLFramebuffer();

  OpenGLFramebuffer(const OpenGLFramebuffer&) = delete;

  ~OpenGLFramebuffer();

  bool saveHDR(const char* path) const;

  bool savePNG(const char* path) const;

  bool isBound() const noexcept { return m_boundFlag; }

  GLenum getStatus() const
  {
    assert(m_boundFlag);

    return glCheckFramebufferStatus(GL_FRAMEBUFFER);
  }

  void assertStatusComplete(const char* sourceFilePath, int sourceFileLine)
  {
#ifndef NDEBUG
    const GLenum status = getStatus();

    if (status != GL_FRAMEBUFFER_COMPLETE) {

      switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
          fprintf(stderr, "%s:%d: GL_FRAMEBUFFER_UNDEFINED\n", sourceFilePath, sourceFileLine);
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
          fprintf(stderr, "%s:%d: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n", sourceFilePath, sourceFileLine);
          break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
          fprintf(stderr, "%s:%d: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n", sourceFilePath, sourceFileLine);
          break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
          fprintf(stderr, "%s:%d: GL_FRAMEBUFFER_UNSUPPORTED\n", sourceFilePath, sourceFileLine);
          break;
      }

      exit(EXIT_FAILURE);
    }
#else
    (void)sourceFilePath;
    (void)sourceFileLine;
#endif // NDEBUG
  }

  /// @note The framebuffer must be bound before calling this function.
  bool isComplete() const;

  void bind();

  void unbind();

  /// @note The framebuffer must be bound before calling this function.
  void attach(OpenGLRenderbuffer&);

  /// @note The framebuffer must be bound before calling this function.
  void attach(OpenGLTexture2D& colorAttachment);

  /// @note The framebuffer must be bound before calling this function.
  void attach(const std::vector<OpenGLTexture2D*>& colorAttachments);

  void read(GLenum attachment,
            GLint x,
            GLint y,
            GLsizei width,
            GLsizei height,
            GLenum format,
            GLenum type,
            GLvoid* data) const
  {
    assert(m_boundFlag);

    glReadBuffer(attachment);

    glReadPixels(x, y, width, height, format, type, data);
  }

  void read(GLenum attachment, GLint x, GLint y, GLsizei width, GLsizei height, glm::vec3* rgb) const
  {
    read(attachment, x, y, width, height, GL_RGB, GL_FLOAT, rgb);
  }

  void read(GLenum attachment, GLint x, GLint y, GLsizei width, GLsizei height, glm::vec4* rgba) const
  {
    read(attachment, x, y, width, height, GL_RGBA, GL_FLOAT, rgba);
  }

private:
  GLuint m_framebufferID = 0;

  bool m_boundFlag = false;
};

} // namespace sf
