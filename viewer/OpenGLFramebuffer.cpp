#include "OpenGLFramebuffer.h"

#include "OpenGLError.h"
#include "OpenGLRenderbuffer.h"
#include "OpenGLTexture2D.h"

#include <glm/vec3.hpp>

#include <cassert>

std::vector<GLenum>
OpenGLFramebuffer::getAttachments(GLenum framebuffer)
{
  std::vector<GLenum> attachments;

  GLint framebufferID = 0;

  switch (framebuffer) {
    case GL_READ_FRAMEBUFFER:
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebufferID);
      break;
    case GL_DRAW_FRAMEBUFFER:
      glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebufferID);
      break;
    default:
      return attachments;
  }

  if (framebufferID == 0) {
    attachments.emplace_back(GL_BACK);
    return attachments;
  }

  for (GLint attachmentIndex = 0; attachmentIndex < GL_MAX_COLOR_ATTACHMENTS; attachmentIndex++) {

    const GLenum attachment = GL_COLOR_ATTACHMENT0 + attachmentIndex;

    GLint objectType = 0;

    glGetFramebufferAttachmentParameteriv(framebuffer, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);

    if (objectType == GL_NONE)
      break;

    attachments.emplace_back(attachment);
  }

  return attachments;
}

Texture<glm::vec4>
OpenGLFramebuffer::readTextureRGBA(GLuint textureID)
{
  if (textureID == 0)
    return Texture<glm::vec4>();

  assert(false);

  GLint w = 0;
  GLint h = 0;

  Texture<glm::vec4> texture(w, h);

  return texture;
}

Texture<glm::vec4>
OpenGLFramebuffer::readAttachmentRGBA(GLenum attachment, GLFWwindow* window)
{
  GLint type = GL_NONE;

  glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);

  if (type == GL_NONE)
    return Texture<glm::vec4>();

  if (type == GL_FRAMEBUFFER_DEFAULT) {

    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window, &w, &h);

    Texture<glm::vec4> texture(w, h);

    OpenGLError::assertNone(__FILE__, __LINE__);

    glReadBuffer(attachment);

    // TODO : This emits GL_INVALID_OPERATION. Why?

    glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, &texture[0]);

    OpenGLError::assertNone(__FILE__, __LINE__);

    return texture;

  } else if (type == GL_TEXTURE) {

    GLint textureID = 0;

    glGetFramebufferAttachmentParameteriv(
      GL_READ_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &textureID);

    return readTextureRGBA(textureID);
  }

  return Texture<glm::vec4>();
}

OpenGLFramebuffer::OpenGLFramebuffer()
{
  glGenFramebuffers(1, &m_framebufferID);
}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
  glDeleteFramebuffers(1, &m_framebufferID);
}

void
OpenGLFramebuffer::bind()
{
  assert(!m_boundFlag);

  glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);

  m_boundFlag = true;
}

void
OpenGLFramebuffer::unbind()
{
  assert(m_boundFlag);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  m_boundFlag = false;
}

void
OpenGLFramebuffer::attach(OpenGLRenderbuffer& renderbuffer)
{
  assert(m_boundFlag);

  assert(renderbuffer.isBound());

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.getRenderbufferID());
}

void
OpenGLFramebuffer::attach(OpenGLTexture2D& texture)
{
  return attach({ &texture });
}

void
OpenGLFramebuffer::attach(const std::vector<OpenGLTexture2D*>& textures)
{
  assert(m_boundFlag);

  assert(textures.size() <= 8);

  std::vector<GLenum> drawBuffers;

  for (int i = 0; (i < int(textures.size())) && (i < 8); i++) {

    const GLenum colorAttachment = GL_COLOR_ATTACHMENT0 + i;

    drawBuffers.emplace_back(colorAttachment);

    glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachment, GL_TEXTURE_2D, textures[i]->id(), 0);
  }

  glDrawBuffers(drawBuffers.size(), &drawBuffers[0]);
}

bool
OpenGLFramebuffer::isComplete() const
{
  assert(m_boundFlag);

  return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}
