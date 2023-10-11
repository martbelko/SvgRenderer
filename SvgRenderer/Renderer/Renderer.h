#pragma once

#include "Renderer/Shader.h"

#include "Scene/OrthographicCamera.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	class Renderer
	{
	public:
		static void Init(uint32_t initWidth, uint32_t initHeight);
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();

		static void DrawTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
	private:
		static Ref<Shader> s_MainShader;
		static const OrthographicCamera* m_ActiveCamera;
	};

}
