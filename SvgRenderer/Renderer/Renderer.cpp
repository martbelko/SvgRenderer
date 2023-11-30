#include "Renderer.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/VertexArray.h"

#include <glad/glad.h>

namespace SvgRenderer {

	Ref<Shader> Renderer::s_MainShader;

	const OrthographicCamera* Renderer::m_ActiveCamera = nullptr;

	void Renderer::Init(uint32_t initWidth, uint32_t initHeight)
	{
		std::filesystem::path shadersPath(Filesystem::AssetsPath() / "shaders");
		s_MainShader = Shader::Create(shadersPath / "main.vert", shadersPath / "main.frag");

		glViewport(0, 0, initWidth, initHeight);

		glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
		glClearStencil(0x0);
		glStencilMask(0xFF);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_STENCIL_TEST);
	}

	void Renderer::Shutdown()
	{
		s_MainShader.reset();
	}

	void Renderer::BeginScene(const OrthographicCamera& camera)
	{
		m_ActiveCamera = &camera;
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::RenderFramebuffer(const Ref<Framebuffer>& fbo)
	{
		Ref<VertexArray> emptyVao = VertexArray::Create();

		emptyVao->Bind();
		glDisable(GL_DEPTH_TEST);
		glBindTextureUnit(0, fbo->GetColorAttachmentRendererID());
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

}
