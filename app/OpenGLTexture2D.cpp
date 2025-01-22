#include "OpenGLTexture2D.h"

#include <stb_image.h>

#include <memory>

#include <cassert>

OpenGLTexture2D::OpenGLTexture2D()
{
  glGenTextures(1, &m_textureID);

  bind();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  unbind();
}

OpenGLTexture2D::OpenGLTexture2D(OpenGLTexture2D&& other)
  : m_textureID(other.m_textureID)
    , m_boundFlag(other.m_boundFlag)
{
  other.m_textureID = 0;
  other.m_boundFlag = false;
}

OpenGLTexture2D::~OpenGLTexture2D()
{
  if (m_textureID)
    glDeleteTextures(1, &m_textureID);
}

bool
OpenGLTexture2D::openFile(const char* path, bool flipVertically)
{
  assert(m_boundFlag);

  int w = 0;
  int h = 0;
  int channelCount = 0;

  stbi_set_flip_vertically_on_load(flipVertically);

  unsigned char* data = stbi_load(path, &w, &h, &channelCount, 0);

  if ((channelCount < 1) || (channelCount > 4)) {
    stbi_image_free(data);
    return false;
  }

  GLenum format = 0;

  switch (channelCount) {
    case 1:
      format = GL_RED;
      break;
    case 2:
      format = GL_RG;
      break;
    case 3:
      format = GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      break;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

  stbi_image_free(data);

  return true;
}

void
OpenGLTexture2D::setMinMagFilters(GLenum minFilter, GLenum magFilter)
{
  assert(m_boundFlag);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void
OpenGLTexture2D::bind()
{
  assert(m_boundFlag == false);

  glBindTexture(GL_TEXTURE_2D, m_textureID);

  m_boundFlag = true;
}

void
OpenGLTexture2D::unbind()
{
  assert(m_boundFlag == true);

  glBindTexture(GL_TEXTURE_2D, 0);

  m_boundFlag = false;
}

void
OpenGLTexture2D::resize(GLint w, GLint h, GLenum internalFormat, GLenum format, GLenum type)
{
  assert(m_boundFlag);

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, nullptr);
}

void
OpenGLTexture2D::write(GLint level, GLint x, GLint y, GLint w, GLint h, GLenum format, GLenum type, const void* pixels)
{
  assert(m_boundFlag);

  glTexSubImage2D(GL_TEXTURE_2D, level, x, y, w, h, format, type, pixels);
}

void
OpenGLTexture2D::write(GLint level, GLint x, GLint y, GLint w, GLint h, const float* r)
{
  write(level, x, y, w, h, GL_RED, GL_FLOAT, r);
}

void
OpenGLTexture2D::write(GLint level, GLint x, GLint y, GLint w, GLint h, const glm::vec3* rgb)
{
  write(level, x, y, w, h, GL_RGB, GL_FLOAT, rgb);
}

void
OpenGLTexture2D::write(GLint level, GLint x, GLint y, GLint w, GLint h, const glm::vec4* rgba)
{
  write(level, x, y, w, h, GL_RGBA, GL_FLOAT, rgba);
}
