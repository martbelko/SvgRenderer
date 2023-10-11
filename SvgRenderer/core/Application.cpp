#include "Application.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/Shader.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace SvgRenderer {

	Application Application::s_Instance;

	void Application::Init()
	{
		m_Window = Window::Create({
			.width = 1280,
			.height = 960,
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
	}

	void Application::Shutdown()
	{
		m_Window->Close();
	}

	void Application::Run()
	{
		std::filesystem::path shadersPath(Filesystem::AssetsPath() / "shaders");
		Ref<Shader> shader = Shader::Create(shadersPath / "main.vert", shadersPath / "main.frag");

		// set up vertex data (and buffer(s)) and configure vertex attributes
		// ------------------------------------------------------------------
		float vertices[] = {
			-0.5f, -0.5f, 0.0f, // left
			 0.5f, -0.5f, 0.0f, // right
			 0.0f,  0.5f, 0.0f  // top
		};


		unsigned int VAO;
		glGenVertexArrays(1, &VAO);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(VAO);

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices, sizeof(vertices));
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
		// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
		glBindVertexArray(0);

		m_Running = true;
		while (m_Running)
		{
			m_Window->OnUpdate();

			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			// draw our first triangle
			shader->Bind();
			glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		glDeleteVertexArrays(1, &VAO);
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
		glViewport(0, 0, width, height);
	}

}
