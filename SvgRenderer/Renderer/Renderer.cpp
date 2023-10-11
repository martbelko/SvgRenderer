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

	void Renderer::DrawTriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2)
	{
		const glm::vec4 color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);
		std::array<Vertex2D, 3> vertices({
			{ p0, color },
			{ p1, color },
			{ p2, color }
		});

		Ref<VertexArray> vao = VertexArray::Create();

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex2D)));
		vbo->SetLayout(Vertex2D::GetBufferLayout());

		vao->AddVertexBuffer(vbo);

		vao->Bind();
		s_MainShader->Bind();

		SR_ASSERT(m_ActiveCamera != nullptr, "Camera was nullptr");
		s_MainShader->SetUniformMat4(0, m_ActiveCamera->GetViewProjectionMatrix());

		glDrawArrays(GL_TRIANGLES, 0, static_cast<uint32_t>(vertices.size()));
	}

	void Renderer::DrawLine(const glm::vec2& start, const glm::vec2& end)
	{
		float strokeWidth = 10.0f;

		glm::vec2 direction = end - start;
		glm::vec2 normalDirection = glm::normalize(direction);
		glm::vec2 perpVector = glm::vec2(-normalDirection.y, normalDirection.x);

		glm::vec2 bottomLeft = start - (perpVector * strokeWidth * 0.5f);
		glm::vec2 bottomRight = start + (perpVector * strokeWidth * 0.5f);
		glm::vec2 topLeft = end + (perpVector * strokeWidth * 0.5f);
		glm::vec2 topRight = end - (perpVector * strokeWidth * 0.5f);

		DrawTriangle(bottomLeft, bottomRight, topLeft);
		DrawTriangle(bottomLeft, topLeft, topRight);
	}

}
