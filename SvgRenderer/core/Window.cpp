#include "Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace SvgRenderer {

	Window::Window(const WindowDesc& desc)
		: m_WindowDescriptor(desc)
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* window = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), NULL, NULL);
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

		SR_ASSERT(desc.callbacks.onWindowClose != nullptr, "Callback function cannot be nullptr");
		SR_ASSERT(desc.callbacks.onKeyPressed != nullptr, "Callback function cannot be nullptr");
		SR_ASSERT(desc.callbacks.onKeyReleased != nullptr, "Callback function cannot be nullptr");
		SR_ASSERT(desc.callbacks.onMousePressed != nullptr, "Callback function cannot be nullptr");
		SR_ASSERT(desc.callbacks.onMouseReleased != nullptr, "Callback function cannot be nullptr");
		SR_ASSERT(desc.callbacks.onViewportSizeChanged != nullptr, "Callback function cannot be nullptr");

		glfwSetWindowUserPointer(window, this);

		glfwSetWindowCloseCallback(window, Window::OnWindowShouldClose);
		glfwSetKeyCallback(window, Window::OnKeyChanged);
		glfwSetMouseButtonCallback(window, Window::OnMouseChanged);
		glfwSetFramebufferSizeCallback(window, Window::OnViewportSizeChanged);

		m_NativeWindow = window;
	}

	void Window::Close()
	{
		glfwDestroyWindow(m_NativeWindow);
		glfwTerminate();
	}

	void Window::OnUpdate()
	{
		glfwSwapBuffers(m_NativeWindow);
		glfwPollEvents();
	}

	void Window::OnWindowShouldClose(GLFWwindow* window)
	{
		Window* wnd = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		wnd->m_WindowDescriptor.callbacks.onWindowClose();
	}

	void Window::OnKeyChanged(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* wnd = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		if (action == GLFW_PRESS)
			wnd->m_WindowDescriptor.callbacks.onKeyPressed(key, 0);
		else if (action == GLFW_REPEAT)
			wnd->m_WindowDescriptor.callbacks.onKeyPressed(key, 1);
		else if (action == GLFW_REPEAT)
			wnd->m_WindowDescriptor.callbacks.onKeyReleased(key);
	}

	void Window::OnMouseChanged(GLFWwindow* window, int button, int action, int mods)
	{
		Window* wnd = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		if (action == GLFW_PRESS)
			wnd->m_WindowDescriptor.callbacks.onMousePressed(button, 0);
		else if (action == GLFW_REPEAT)
			wnd->m_WindowDescriptor.callbacks.onMousePressed(button, 1);
		else if (action == GLFW_REPEAT)
			wnd->m_WindowDescriptor.callbacks.onMouseReleased(button);
	}

	void Window::OnViewportSizeChanged(GLFWwindow* window, int width, int height)
	{
		Window* wnd = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		wnd->m_WindowDescriptor.callbacks.onViewportSizeChanged(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

}
