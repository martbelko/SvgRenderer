#include "UniformBuffer.h"

#include <glad/glad.h>

namespace SvgRenderer {

	UniformBuffer::~UniformBuffer()
	{
		glDeleteBuffers(1, &m_RendererId);
	}

	void UniformBuffer::Bind(uint32_t location)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, location, m_RendererId);
	}

	void UniformBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		glNamedBufferSubData(m_RendererId, offset, size, data);
	}

	void UniformBuffer::ReadData(void* data, uint64_t size, uint64_t offset) const
	{
		glGetNamedBufferSubData(m_RendererId, offset, size, data);
	}

	Ref<UniformBuffer> UniformBuffer::CreateFromSize(uint64_t size)
	{
		uint32_t ssbo;
		glCreateBuffers(1, &ssbo);
		glNamedBufferData(ssbo, size, nullptr, GL_DYNAMIC_READ);

		return CreateRef<UniformBuffer>(ssbo);
	}

}
