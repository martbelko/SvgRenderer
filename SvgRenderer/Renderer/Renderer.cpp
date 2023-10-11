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

	void Renderer::DrawTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
	{
		const std::array<glm::vec3, 3> vertices = { p1, p2, p3 };

		Ref<VertexArray> vao = VertexArray::Create();

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(glm::vec3));
		vbo->SetLayout({
			{ ShaderDataType::Float3 }
		});

		vao->AddVertexBuffer(vbo);

		vao->Bind();
		s_MainShader->Bind();

		SR_ASSERT(m_ActiveCamera != nullptr, "Camera was nullptr");
		s_MainShader->SetUniformMat4(0, m_ActiveCamera->GetViewProjectionMatrix());

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	}

}
