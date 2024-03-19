#pragma once

#include "Renderer/Pipeline/Pipeline.h"
#include "Renderer/Shader.h"
#include "Renderer/TileBuilder.h"

namespace SvgRenderer {

	enum class CPUMode
	{
		Seq = 0, Par
	};

	class CPUPipeline : public Pipeline
	{
	public:
		CPUPipeline(CPUMode cpuMode)
			: m_CpuMode(cpuMode) {}

		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void Render() override;
		virtual void Final() override;
	private:
		template <class FwdIt, class Fn>
		void ForEach(FwdIt first, FwdIt last, Fn func) noexcept
		{
			if (m_CpuMode == CPUMode::Seq)
			{
				std::for_each(std::execution::seq, first, last, func);
			}
			else
			{
				std::for_each(std::execution::par, first, last, func);
			}
		}
	private:
		TileBuilder m_TileBuilder;
		uint32_t m_Vbo = 0, m_Ibo = 0, m_Vao = 0, m_AlphaTexture = 0;
		Ref<Shader> m_FinalShader;
		uint32_t m_RenderIndicesCount = 0;
		CPUMode m_CpuMode;
	};

}