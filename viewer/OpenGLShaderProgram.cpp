#include "OpenGLShaderProgram.h"

bool
OpenGLShaderProgram::makeSimpleProgram(const char* vertSource, const char* fragSource, std::ostream& errStream)
{
  OpenGLShader vertShader(GL_VERTEX_SHADER);

  vertShader << vertSource;

  vertShader.compile();

  if (!vertShader.isCompiled()) {
    errStream << vertShader.getInfoLog();
    return false;
  }

  OpenGLShader fragShader(GL_FRAGMENT_SHADER);

  fragShader << fragSource;

  fragShader.compile();

  if (!fragShader.isCompiled()) {
    errStream << fragShader.getInfoLog();
    return false;
  }

  attach(vertShader);
  attach(fragShader);

  link();

  detach(vertShader);
  detach(fragShader);

  if (!isLinked()) {
    errStream << getInfoLog();
    return false;
  }

  return true;
}
