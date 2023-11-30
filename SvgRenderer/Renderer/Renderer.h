#pragma once

#include "Renderer/VertexBuffer.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"

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

		static void RenderFramebuffer(const Ref<Framebuffer>& fbo);

		static const OrthographicCamera* GetCurrentCamera3D() { return m_ActiveCamera; }
	private:
		static Ref<Shader> s_MainShader;

		static const OrthographicCamera* m_ActiveCamera;
	};

}
