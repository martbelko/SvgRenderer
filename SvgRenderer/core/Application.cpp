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

	struct QuadBez
	{
		glm::vec2 p0, p1, p2;
	};

	struct LineBez
	{
		glm::vec2 p0, p1;
	};

	struct CurvedPolygon
	{
		std::vector<LineBez> lines;
		std::vector<QuadBez> bezs;
	};

	OrthographicCamera camera(0, 1280.0f, 0.0f, 720.0f);

	void Application::DrawVertices(const CurvedPolygon& polygon, const glm::vec2& centroid, const Ref<Shader>& shader, float ref)
	{
		std::vector<glm::vec2> vertices;
		for (const LineBez& line : polygon.lines)
		{
			glm::vec2 p0 = centroid;
			glm::vec2 p1 = line.p0;
			glm::vec2 p2 = line.p1;

			float s = p0.x * p1.y + p1.x * p2.y + p2.x * p0.y - p1.x * p0.y - p0.x * p2.y - p2.x * p1.y;
			if (s > 0.0f)
				s = 1.0f;
			else if (s < 0.0f)
				s = -1.0f;
			else
				s = 0.0f;

			vertices.push_back(centroid);
			vertices.push_back({ s, s });
			vertices.push_back(line.p0);
			vertices.push_back({ s, s });
			vertices.push_back(line.p1);
			vertices.push_back({ s, s });
		}

		for (const QuadBez& bez : polygon.bezs)
		{
			glm::vec2 p0 = centroid;
			glm::vec2 p1 = bez.p0;
			glm::vec2 p2 = bez.p2;

			float s = p0.x * p1.y + p1.x * p2.y + p2.x * p0.y - p1.x * p0.y - p0.x * p2.y - p2.x * p1.y;
			if (s > 0.0f)
				s = 1.0f;
			else if (s < 0.0f)
				s = -1.0f;
			else
				s = 0.0f;

			vertices.push_back(p0);
			vertices.push_back({ s, s });
			vertices.push_back(p1);
			vertices.push_back({ s, s });
			vertices.push_back(p2);
			vertices.push_back({ s, s });
		}

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(glm::vec2));
		vbo->SetLayout({
			{ ShaderDataType::Float2 },
			{ ShaderDataType::Float2 }
			});

		Ref<VertexArray> vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);

		vao->Bind();
		shader->Bind();
		shader->SetUniformMat4(0, camera.GetViewProjectionMatrix());

		glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
	}

	void Application::DrawVerticesBezier(const CurvedPolygon& polygon, const glm::vec2& centroid, const Ref<Shader>& shader, float ref)
	{
		std::vector<glm::vec2> vertices;
		for (const QuadBez& bez : polygon.bezs)
		{
			glm::vec2 p0 = bez.p0;
			glm::vec2 p1 = bez.p1;
			glm::vec2 p2 = bez.p2;

			float s = p0.x * p1.y + p1.x * p2.y + p2.x * p0.y - p1.x * p0.y - p0.x * p2.y - p2.x * p1.y;
			if (s > 0.0f)
				s = 1.0f;
			else if (s < 0.0f)
				s = -1.0f;
			else
				s = 0.0f;

			vertices.push_back(p0);
			vertices.push_back({ s, s });
			vertices.push_back(p1);
			vertices.push_back({ s, s });
			vertices.push_back(p2);
			vertices.push_back({ s, s });
		}

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(glm::vec2));
		vbo->SetLayout({
			{ ShaderDataType::Float2 },
			{ ShaderDataType::Float2 }
		});

		Ref<VertexArray> vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);

		vao->Bind();
		shader->Bind();
		shader->SetUniformMat4(0, camera.GetViewProjectionMatrix());

		glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
	}

	Ref<Shader> triangleShader;
	Ref<Shader> triangle2Shader;
	Ref<Shader> bezierShader;
	Ref<Shader> bezier2Shader;

	void Application::DrawPolygon(const CurvedPolygon& polygon)
	{
		glm::vec2 centroid = glm::vec2(0, 0);
		for (const LineBez& l : polygon.lines)
		{
			centroid += l.p0;
			centroid += l.p1;
		}

		for (const QuadBez& q : polygon.bezs)
		{
			centroid += q.p0;
			centroid += q.p1;
			centroid += q.p2;
		}
		centroid /= polygon.lines.size() * 2 + polygon.bezs.size() * 3;
		centroid = { 0, 0 };

		glStencilMask(0xFF);
		glStencilFunc(GL_ALWAYS, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
		DrawVertices(polygon, centroid, triangleShader, 1.0f);
		DrawVerticesBezier(polygon, centroid, bezierShader, 5.0f);
	}

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

		triangleShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "tri.vert", Filesystem::AssetsPath() / "shaders" / "tri.frag");
		triangle2Shader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "tri.vert", Filesystem::AssetsPath() / "shaders" / "tri2.frag");
		bezierShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "bez.vert", Filesystem::AssetsPath() / "shaders" / "bez.frag");
		bezier2Shader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "bez.vert", Filesystem::AssetsPath() / "shaders" / "bez2.frag");

		CurvedPolygon polygon;
		polygon.lines.push_back(LineBez{ .p0 = { 100, 100 }, .p1 = { 690, 290 } });

		polygon.bezs.push_back(QuadBez{ .p0 = { 690, 290 }, .p1 = { 600, 600 }, .p2 = { 500, 300 } });

		// polygon.lines.push_back(LineBez{ .p0 = { 690, 290 }, .p1 = { 600, 600 } });
		// polygon.lines.push_back(LineBez{ .p0 = { 600, 600 }, .p1 = { 500, 300 } });
		// polygon.lines.push_back(LineBez{ .p0 = { 690, 290 }, .p1 = { 500, 300 } });

		polygon.lines.push_back(LineBez{ .p0 = { 500, 300 }, .p1 = { 100, 100 } });

		auto PreprocessPolygon = [](CurvedPolygon& p)
			{
				// for (const QuadBez& bez : p.bezs)
				// {
				// 	p.lines.push_back(LineBez{ .p0 = bez.p0, .p1 = bez.p1 });
				// 	p.lines.push_back(LineBez{ .p0 = bez.p1, .p1 = bez.p2 });
				// 	p.lines.push_back(LineBez{ .p0 = bez.p0, .p1 = bez.p2 });
				// }
			};

		PreprocessPolygon(polygon);

		// M 650 300 Q 625 150 600 300 Z
		CurvedPolygon p;
		//p.bezs.push_back(QuadBez{ .p0 = { 650, 300 }, .p1 = { 625, 150 }, .p2 = { 600, 300 } });
		//p.lines.push_back(LineBez{ .p0 = { 600, 300 }, .p1 = { 650, 300 } });
		p.bezs.push_back(QuadBez{ .p0 = { 690, 290 }, .p1 = { 600, 600 }, .p2 = { 500, 300 } });
		p.lines.push_back(LineBez{ .p0 = { 500, 300 }, .p1 = { 690, 290 } });
		PreprocessPolygon(p);

		// M 100 100 L 690 290 Q 600 400 500 300 Z M 650 300 Q 625 150 600 300 Z
		CurvedPolygon pp;
		pp.lines.push_back(LineBez{ .p0 = { 100, 100 }, .p1 = { 690, 290 } });
		pp.bezs.push_back(QuadBez{ .p0 = { 690, 290 }, .p1 = { 600, 400 }, .p2 = { 500, 300 } });
		pp.lines.push_back(LineBez{ .p0 = { 500, 300 }, .p1 = { 100, 100 } });

		pp.bezs.push_back(QuadBez{ .p0 = { 700, 300 }, .p1 = { 650, 450 }, .p2 = { 600, 300 } });
		pp.lines.push_back(LineBez{ .p0 = { 600, 300 }, .p1 = { 700, 300 } });

		m_Running = true;
		glDisable(GL_DEPTH_TEST);
		while (m_Running)
		{
			fbo->Bind();

			// Renderer::BeginScene(camera);
			//
			// Path2DContext* ctx = Renderer::BeginPath(glm::vec2(100, 100), glm::mat4(1.0f));
			// Renderer::LineTo(ctx, glm::vec2(690, 290));
			// Renderer::QuadTo(ctx, glm::vec2(600, 600), glm::vec2(500, 300));
			// Renderer::EndPath(ctx, true);
			// delete ctx;
			//
			// Renderer::EndScene();

			glEnable(GL_STENCIL_TEST);
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClearStencil(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			//DrawPolygon(polygon);
			//DrawPolygon(p);
			DrawPolygon(pp);

			glBlitNamedFramebuffer(fbo->GetRendererId(), 0,
				0, 0, 1280, 720,
				0, 0, 1280, 720,
				GL_STENCIL_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			//glStencilMask(0x00);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
			//glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
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
