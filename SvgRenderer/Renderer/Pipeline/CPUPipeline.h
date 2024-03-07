#pragma once

#include "Renderer/Pipeline/Pipeline.h"
#include "Renderer/Shader.h"
#include "Renderer/TileBuilder.h"

namespace SvgRenderer {

	class CPUPipeline : public Pipeline
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void Render() override;
		virtual void Final() override;
	private:
		TileBuilder m_TileBuilder;
		uint32_t m_Vbo, m_Ibo, m_Vao, m_AlphaTexture;
		Ref<Shader> m_FinalShader;
		uint32_t m_RenderIndicesCount;
	};

}