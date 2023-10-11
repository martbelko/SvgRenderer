#pragma once

namespace SvgRenderer {

	class IndexBuffer
	{
	public:
		IndexBuffer(uint32_t* indices, uint32_t count);
		~IndexBuffer();

		uint32_t GetCount() const { return m_Count; }
	public:
		static Ref<IndexBuffer> Create(uint32_t* indices, uint32_t count)
		{
			return CreateRef<IndexBuffer>(indices, count);
		}
	private:
		uint32_t m_RendererId;
		uint32_t m_Count;
	};

}
