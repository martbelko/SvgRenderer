#pragma once

#include "Renderer/Pipeline/Pipeline.h"
#include "Renderer/Shader.h"
#include "Renderer/TileBuilder.h"

namespace SvgRenderer {

	class GPUPipeline : public Pipeline
	{
		struct ParamsBuf
		{
			glm::mat4 globalTransform;
			uint32_t screenWidth;
			uint32_t screenHeight;
		};
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void Render() override;
		virtual void Final() override;
	private:
		template <class FwdIt, class Fn>
		void ForEach(FwdIt first, FwdIt last, Fn func) noexcept
		{
			std::for_each(std::execution::seq, first, last, func);
		}
	private:
		TileBuilder m_TileBuilder;
		uint32_t m_Vbo = 0, m_Ibo = 0, m_Vao = 0, m_AlphaTexture = 0;
		uint32_t m_RenderIndicesCount = 0;

		Ref<Shader> m_FinalShader;
		Ref<Shader> m_ResetShader;
		Ref<Shader> m_TransformShader;
		Ref<Shader> m_CoarseBboxShader;
		Ref<Shader> m_PreFlattenShader;
		Ref<Shader> m_FlattenShader;

		uint32_t m_ParamsBuf, m_PathsBuf, m_CmdsBuf, m_SimpleCmdsBuf, m_TilesBuf, m_VerticesBuf, m_AtlasBuf, m_TempBuf;

		ParamsBuf m_Params;
	};

}