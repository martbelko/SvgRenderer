#pragma once

#include "Renderer/Pipeline/Pipeline.h"
#include "Renderer/Shader.h"
#include "Renderer/TileBuilder.h"

namespace SvgRenderer {

	class GPUPipeline : public Pipeline
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

		Ref<Shader> shaderTransform;
		Ref<Shader> shaderPreFlatten;
		Ref<Shader> shaderFlatten;
		Ref<Shader> shaderPreFill;
		Ref<Shader> shaderFill;
		Ref<Shader> shaderCalculateQuads;
		Ref<Shader> shaderPrefixSum;
		Ref<Shader> shaderCoarse;
		Ref<Shader> shaderFine;
	};

}
