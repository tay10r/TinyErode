#pragma once

#include <glad/glad.h>

namespace sf {

class OpenGLRenderbuffer final
{
public:
  OpenGLRenderbuffer();

  OpenGLRenderbuffer(const OpenGLRenderbuffer&) = delete;

  ~OpenGLRenderbuffer();

  bool isBound() const noexcept { return m_boundFlag; }

  void bind();

  void unbind();

  void resize(int w, int h, GLenum internalformat = GL_DEPTH_COMPONENT32F);

  GLuint getRenderbufferID() noexcept { return m_renderbufferID; }

private:
  GLuint m_renderbufferID = 0;

  bool m_boundFlag = false;
};

} // namespace sf
