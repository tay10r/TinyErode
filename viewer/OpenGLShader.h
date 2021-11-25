#pragma once

#include <glad/glad.h>

#include <sstream>

#include <cassert>

class OpenGLShader final
{
public:
  OpenGLShader(GLenum kind)
    : m_shaderID(glCreateShader(kind))
  {}

  ~OpenGLShader()
  {
    if (m_shaderID)
      glDeleteShader(m_shaderID);
  }

  GLuint getShaderID() { return m_shaderID; }

  template<typename SourceType>
  OpenGLShader& operator<<(const SourceType& source)
  {
    m_sourceStream << source;
    return *this;
  }

  GLint getIntegerValue(GLenum kind) const
  {
    GLint value = 0;

    glGetShaderiv(m_shaderID, kind, &value);

    return value;
  }

  GLint getCompileStatus() const { return getIntegerValue(GL_COMPILE_STATUS); }

  bool isCompiled() const { return getCompileStatus() != 0; }

  GLint getInfoLogLength() const { return getIntegerValue(GL_INFO_LOG_LENGTH); }

  std::string getInfoLog() const
  {
    std::string infoLog;

    const GLint logLength = getInfoLogLength();

    if (logLength < 0)
      return infoLog;

    infoLog.resize(size_t(logLength));

    GLsizei finalLength = 0;

    glGetShaderInfoLog(m_shaderID, logLength, &finalLength, &infoLog[0]);

    if (finalLength < 0)
      infoLog.clear();
    else
      infoLog.resize(size_t(finalLength));

    return infoLog;
  }

  void compile()
  {
    const std::string source = m_sourceStream.str();

    const GLchar* ptr = source.c_str();

    GLint length = source.size();

    glShaderSource(m_shaderID, 1, &ptr, &length);

    glCompileShader(m_shaderID);
  }

private:
  std::ostringstream m_sourceStream;

  GLuint m_shaderID = 0;
};
