#include "Application.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/FillRenderer.h"

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

	OrthographicCamera camera(0, 1280.0f, 0.0f, 720.0f, -100.0f, 100.0f);

	void Application::Run()
	{
		camera.SetPosition(glm::vec3(0, 0, 0.5f));

		FramebufferDesc fbDesc;
		fbDesc.width = 1280;
		fbDesc.height = 720;
		fbDesc.attachments = {
			{ FramebufferTextureFormat::RGBA8 },
			{ FramebufferTextureFormat::DEPTH24STENCIL8 }
		};

		Ref<Framebuffer> fbo = Framebuffer::Create(fbDesc);

		m_Running = true;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FillRenderer fillRenderer;
		fillRenderer.Init();

		FillPath path1 = FillPathBuilder().MoveTo({ 50, 50 }).QuadTo({ 610, 700 }, { 1220, 50 }).LineTo({ 50, 50 })     .Build();
		FillPath path2 = FillPathBuilder().MoveTo({ 50, 700 }).QuadTo({ 250, 500 }, { 450, 700 }).LineTo({ 50, 700 })   .Build();
		FillPath path3 = FillPathBuilder().MoveTo({ 750, 700 }).QuadTo({ 950, 500 }, { 1150, 700 }).LineTo({ 750, 700 }).Build();

		while (m_Running)
		{
			fbo->Bind();

			glEnable(GL_STENCIL_TEST);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClearStencil(0); // Change this to glClearStencil(100) for nonzero fill rule

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			glStencilMask(0xFF);
			glStencilFunc(GL_ALWAYS, 0, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

			fillRenderer.BeginScene(camera);

			fillRenderer.DrawPath(path1);
			fillRenderer.DrawPath(path2);
			fillRenderer.DrawPath(path3);

			fillRenderer.EndScene();

			// glClear(GL_COLOR_BUFFER_BIT);

			glBlitNamedFramebuffer(fbo->GetRendererId(), 0,
				0, 0, 1280, 720,
				0, 0, 1280, 720,
				GL_STENCIL_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glStencilMask(0x00);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			//glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilFunc(GL_NOTEQUAL, 0, 0xFF); // Change this to glStencilFunc(GL_NOTEQUAL, 100, 0xFF) for nonzero fill rule
			Renderer::RenderFramebuffer(fbo);

			// glDisable(GL_STENCIL_TEST);
			//
			// Renderer::BeginScene(camera);
			//
			// Path2DContext* ctx = Renderer::BeginPath(glm::vec2(100, 100), glm::mat4(1.0f));
			// Renderer::LineTo(ctx, glm::vec2(690, 290));
			// Renderer::QuadTo(ctx, glm::vec2(600, 600), glm::vec2(500, 300));
			// Renderer::EndPath(ctx, true);
			// delete ctx;
			//
			// Renderer::EndScene();

			// FillPath path = FillPathBuilder().MoveTo().LineTo().LineTo().Close().MoveTo().QuadTo().Build();

			m_Window->OnUpdate();
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
