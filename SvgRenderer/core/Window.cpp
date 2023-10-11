#include "Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace SvgRenderer {

	Window::Window(uint32_t width, uint32_t height, const std::string& title)
		: m_Width(width), m_Height(height)
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		if (!window)
		{
			glfwTerminate();
			SR_VERIFY(false, "Failed to create GLFW window");
		}

		glfwMakeContextCurrent(window);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			glfwTerminate();
			SR_VERIFY(false, "Failed to initialize GLAD");
		}

		glfwSetWindowUserPointer(window, this);
		glfwSetWindowCloseCallback(window, [](GLFWwindow* window)
		{
		});

		m_NativeWindow = window;
	}

	Window::~Window()
	{
		glfwTerminate();
	}

	void Window::OnUpdate()
	{
		glfwSwapBuffers(m_NativeWindow);
		glfwPollEvents();
	}

}
