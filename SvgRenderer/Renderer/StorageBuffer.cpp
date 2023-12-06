#include "StorageBuffer.h"

#include <glad/glad.h>

namespace SvgRenderer {

	StorageBuffer::~StorageBuffer()
	{
		glDeleteBuffers(1, &m_RendererId);
	}

	void StorageBuffer::Bind(uint32_t location)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, m_RendererId);;
	}

	void StorageBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		glNamedBufferSubData(m_RendererId, offset, size, data);
	}

	void StorageBuffer::ReadData(void* data, uint64_t size, uint64_t offset) const
	{
		glGetNamedBufferSubData(m_RendererId, offset, size, data);
	}

	Ref<StorageBuffer> StorageBuffer::CreateFromSize(uint64_t size)
	{
		uint32_t ssbo;
		glCreateBuffers(1, &ssbo);
		glNamedBufferData(ssbo, size, nullptr, GL_DYNAMIC_READ);

		return CreateRef<StorageBuffer>(ssbo);
	}

}
