#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <cassert>
#include <cstddef>

template<typename Attrib>
struct OpenGLVertexAttribTraits final
{};

template<>
struct OpenGLVertexAttribTraits<float> final
{
  static constexpr GLint tupleSize() { return 1; }

  static constexpr GLint byteSize() { return 4; }

  static constexpr GLenum type() { return GL_FLOAT; }

  static constexpr GLboolean normalized() { return GL_FALSE; }
};

template<>
struct OpenGLVertexAttribTraits<glm::vec2> final
{
  static constexpr GLint tupleSize() { return 2; }

  static constexpr GLint byteSize() { return 8; }

  static constexpr GLenum type() { return GL_FLOAT; }

  static constexpr GLboolean normalized() { return GL_FALSE; }
};

template<>
struct OpenGLVertexAttribTraits<glm::vec3> final
{
  static constexpr GLint tupleSize() { return 3; }

  static constexpr GLint byteSize() { return 12; }

  static constexpr GLenum type() { return GL_FLOAT; }

  static constexpr GLboolean normalized() { return GL_FALSE; }
};

template<>
struct OpenGLVertexAttribTraits<glm::vec4> final
{
  static constexpr GLint tupleSize() { return 4; }

  static constexpr GLint byteSize() { return 16; }

  static constexpr GLenum type() { return GL_FLOAT; }

  static constexpr GLboolean normalized() { return GL_FALSE; }
};

template<typename Attrib, typename... Attribs>
struct OpenGLVertex final
{
  static constexpr GLint byteSize()
  {
    return OpenGLVertexAttribTraits<Attrib>::byteSize() + OpenGLVertex<Attribs...>::byteSize();
  }

  static void init(GLsizei stride, int index = 0, unsigned char* ptrOffset = nullptr)
  {
    using Traits = OpenGLVertexAttribTraits<Attrib>;

    glEnableVertexAttribArray(index);

    glVertexAttribPointer(index, Traits::tupleSize(), Traits::type(), Traits::normalized(), stride, ptrOffset);

    OpenGLVertex<Attribs...>::init(stride, index + 1, ptrOffset + Traits::byteSize());
  }
};

template<typename Attrib>
struct OpenGLVertex<Attrib> final
{
  static constexpr GLint byteSize() { return OpenGLVertexAttribTraits<Attrib>::byteSize(); }

  static void init(GLsizei stride, int index = 0, unsigned char* ptrOffset = nullptr)
  {
    using Traits = OpenGLVertexAttribTraits<Attrib>;

    glEnableVertexAttribArray(index);

    glVertexAttribPointer(index, Traits::tupleSize(), Traits::type(), Traits::normalized(), stride, ptrOffset);
  }
};

template<typename... Attribs>
class OpenGLVertexBuffer final
{
public:
  OpenGLVertexBuffer();

  OpenGLVertexBuffer(OpenGLVertexBuffer&&);

  OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;

  ~OpenGLVertexBuffer();

  void draw(GLenum primitive, GLint first = 0) const { draw(primitive, first, getVertexCount()); }

  void draw(GLenum primitive, GLint first, GLint count) const { glDrawArrays(primitive, first, count); }

  void bind();

  void unbind();

  bool isBound() const noexcept { return m_boundFlag; }

  /// Resizes the buffer to fit a given number of vertices.
  ///
  /// @param vertexCount The number of vertices to allocate room for.
  void allocate(size_t vertexCount, GLenum usage);

  /// @note Must be allocated with @ref OpenGLVertexBuffer::resize before calling this function.
  ///
  /// @param offset The index of the vertex to start writing the data to.
  ///
  /// @param data The vertices to write to the buffer.
  ///
  /// @param vertexCount The number of vertices to write to the buffer.
  void write(size_t offset, const void* data, size_t vertexCount);

  /// Gets the number of vertices in the buffer.
  ///
  /// @note The vertex buffer must be bound before calling this function.
  size_t getVertexCount() const;

private:
  GLuint m_vertexArrayObject = 0;

  GLuint m_vertexBuffer = 0;

  bool m_boundFlag = false;
};

template<typename... Attribs>
OpenGLVertexBuffer<Attribs...>::OpenGLVertexBuffer()
{
  glGenVertexArrays(1, &m_vertexArrayObject);

  glBindVertexArray(m_vertexArrayObject);

  glGenBuffers(1, &m_vertexBuffer);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

  OpenGLVertex<Attribs...>::init(OpenGLVertex<Attribs...>::byteSize());

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);
}

template<typename... Attribs>
OpenGLVertexBuffer<Attribs...>::OpenGLVertexBuffer(OpenGLVertexBuffer<Attribs...>&& other)
  : m_vertexArrayObject(other.m_vertexArrayObject)
  , m_vertexBuffer(other.m_vertexBuffer)
  , m_boundFlag(other.m_boundFlag)
{
  other.m_vertexArrayObject = 0;
  other.m_vertexBuffer = 0;
  other.m_boundFlag = false;
}

template<typename... Attribs>
OpenGLVertexBuffer<Attribs...>::~OpenGLVertexBuffer()
{
  if (m_vertexBuffer)
    glDeleteBuffers(1, &m_vertexBuffer);

  if (m_vertexArrayObject)
    glDeleteVertexArrays(1, &m_vertexArrayObject);
}

template<typename... Attribs>
void
OpenGLVertexBuffer<Attribs...>::allocate(size_t vertexCount, GLenum usage)
{
  assert(m_boundFlag);

  const size_t totalSize = vertexCount * OpenGLVertex<Attribs...>::byteSize();

  glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, usage);
}

template<typename... Attribs>
void
OpenGLVertexBuffer<Attribs...>::write(size_t offset, const void* vertices, size_t vertexCount)
{
  assert(m_boundFlag);

  using Vertex = OpenGLVertex<Attribs...>;

  const size_t byteOffset = Vertex::byteSize() * offset;

  const size_t byteCount = Vertex::byteSize() * vertexCount;

  glBufferSubData(GL_ARRAY_BUFFER, byteOffset, byteCount, vertices);
}

template<typename... Attribs>
void
OpenGLVertexBuffer<Attribs...>::bind()
{
  assert(!m_boundFlag);

  glBindVertexArray(m_vertexArrayObject);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);

  m_boundFlag = true;
}

template<typename... Attribs>
void
OpenGLVertexBuffer<Attribs...>::unbind()
{
  assert(m_boundFlag);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);

  m_boundFlag = false;
}

template<typename... Attribs>
size_t
OpenGLVertexBuffer<Attribs...>::getVertexCount() const
{
  assert(m_boundFlag);

  GLint size = 0;

  glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

  return size_t(size) / OpenGLVertex<Attribs...>::byteSize();
}
