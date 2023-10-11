#include "srpch.h"
#include "Application.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace SvgRenderer {

	Application Application::s_Instance;

	void Application::Init()
	{
		m_Window = CreateScope<Window>();
	}

	void Application::Shutdown()
	{

	}

	void Application::Run()
	{
		m_Running = true;
		while (m_Running)
		{
			m_Window->OnUpdate();

			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}

	void Application::OnWindowClose()
	{

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
