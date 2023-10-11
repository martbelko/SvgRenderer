#include "VertexBuffer.h"

#include <glad/glad.h>

namespace SvgRenderer {

	VertexBuffer::VertexBuffer(const void* vertices, uint32_t size)
	{
		glCreateBuffers(1, &m_RendererId);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	VertexBuffer::VertexBuffer(uint32_t size)
	{
		glCreateBuffers(1, &m_RendererId);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	VertexBuffer::~VertexBuffer()
	{
		glDeleteBuffers(1, &m_RendererId);
	}

	void VertexBuffer::Bind() const
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
	}

	void VertexBuffer::SetData(const void* data, uint32_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

}
