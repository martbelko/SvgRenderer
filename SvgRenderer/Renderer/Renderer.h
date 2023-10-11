#pragma once

#include "Renderer/VertexBuffer.h"
#include "Renderer/Shader.h"

#include "Scene/OrthographicCamera.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	struct Vertex2D
	{
		glm::vec2 position;
		glm::vec4 color;

		static BufferLayout GetBufferLayout()
		{
			return BufferLayout({
				{ ShaderDataType::Float2 },
				{ ShaderDataType::Float4 }
			});
		}
	};

	class Renderer
	{
	public:
		static void Init(uint32_t initWidth, uint32_t initHeight);
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();

		static void DrawTriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2);
		static void DrawLine(const glm::vec2& from, const glm::vec2& to);
	private:
		static Ref<Shader> s_MainShader;
		static const OrthographicCamera* m_ActiveCamera;
	};

}
