#include "OpenGLRenderbuffer.h"

#include <cassert>

namespace sf {

OpenGLRenderbuffer::OpenGLRenderbuffer()
{
  glGenRenderbuffers(1, &m_renderbufferID);
}

OpenGLRenderbuffer::~OpenGLRenderbuffer()
{
  if (m_renderbufferID)
    glDeleteRenderbuffers(1, &m_renderbufferID);
}

void
OpenGLRenderbuffer::bind()
{
  assert(!m_boundFlag);

  glBindRenderbuffer(GL_RENDERBUFFER, m_renderbufferID);

  m_boundFlag = true;
}

void
OpenGLRenderbuffer::unbind()
{
  assert(m_boundFlag);

  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  m_boundFlag = false;
}

void
OpenGLRenderbuffer::resize(int w, int h, GLenum internalFormat)
{
  assert(m_boundFlag);

  glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, w, h);
}

} // namespace sf
