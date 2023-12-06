#pragma once

#include "Core/Base.h"

#include <cstdint>

namespace SvgRenderer {

	class UniformBuffer
	{
	public:
		UniformBuffer(uint32_t rendererId)
			: m_RendererId(rendererId) {}
		~UniformBuffer();

		void Bind(uint32_t location);

		void SetData(const void* data, uint64_t size, uint64_t offset = 0);
		void ReadData(void* data, uint64_t size, uint64_t offset = 0) const;
	public:
		static Ref<UniformBuffer> CreateFromSize(uint64_t size);
	private:
		uint32_t m_RendererId;
	};

}
