#include "Application.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Renderer.h"

#include "Scene/OrthographicCamera.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>

namespace SvgRenderer {

	Application Application::s_Instance;

	void Application::Init()
	{
		uint32_t initWidth = 1280, initHeight = 720;

		m_Window = Window::Create({
			.width = initWidth,
			.height = initHeight,
			.title = "SvgRenderer",
			.callbacks = {
				.onWindowClose = Application::OnWindowCloseStatic,
				.onKeyPressed = Application::OnKeyPressedStatic,
				.onKeyReleased = Application::OnKeyReleasedStatic,
				.onMousePressed = Application::OnMousePressedStatic,
				.onMouseReleased = Application::OnMouseReleasedStatic,
				.onViewportSizeChanged = Application::OnViewportResizeStatic
			}
		});

		Renderer::Init(initWidth, initHeight);
	}

	void Application::Shutdown()
	{
		Renderer::Shutdown();
		m_Window->Close();
	}

	void Application::Run()
	{
		OrthographicCamera camera(0, 1280.0f, 0.0f, 720.0f);
		camera.SetPosition(glm::vec3(0, 0, 0.5f));

		FramebufferDesc fbDesc;
		fbDesc.width = 1280;
		fbDesc.height = 720;
		fbDesc.attachments = {
			{ FramebufferTextureFormat::RGBA8 }
		};

		Ref<Framebuffer> fbo = Framebuffer::Create(fbDesc);

		m_Running = true;
		while (m_Running)
		{
			m_Window->OnUpdate();

			fbo->Bind();
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			Renderer::BeginScene(camera);

			// Renderer::DrawTriangle(glm::vec2(0.0f, 720.0f), glm::vec2(0.0f, 100.0f), glm::vec2(1280.0f, 100.0f));
			// Renderer::DrawLine(glm::vec2(100.0f, 50.0f), glm::vec2(1000.0f, 500.0f));
			// Renderer::DrawQuad({ 1000.0f, 500.0f }, { 100.0f, 100.0f });

			Path2DContext* ctx = Renderer::BeginPath(glm::vec2(100, 100), glm::mat4(1.0f));
			Renderer::LineTo(ctx, glm::vec2(690, 290));
			Renderer::QuadTo(ctx, glm::vec2(600, 400), glm::vec2(500, 300));
			Renderer::EndPath(ctx, true);

			delete ctx;

			Renderer::EndScene();

			Renderer::RenderFramebuffer(fbo);
		}
	}

	void Application::OnWindowClose()
	{
		m_Running = false;
	}

	void Application::OnKeyPressed(int key, int repeat)
	{
	}

	void Application::OnKeyReleased(int key)
	{
	}

	void Application::OnMousePressed(int key, int repeat)
	{
	}

	void Application::OnMouseReleased(int key)
	{
	}

	void Application::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

}
