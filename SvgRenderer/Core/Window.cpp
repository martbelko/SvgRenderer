#include "Window.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace SvgRenderer {

	void APIENTRY glDebugOutput(GLenum source,
		GLenum type,
		unsigned int id,
		GLenum severity,
		GLsizei length,
		const char* message,
		const void* userParam)
	{
		// ignore non-significant error/warning codes
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

		std::cout << "---------------" << std::endl;
		std::cout << "Debug message (" << id << "): " << message << std::endl;

		switch (source)
		{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
		} std::cout << std::endl;

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
		} std::cout << std::endl;

		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
		} std::cout << std::endl;
		std::cout << std::endl;
	}

	Window::Window(const WindowDesc& desc)
		: m_WindowDescriptor(desc)
	{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef _DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#else
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, false);
#endif

		glfwWindowHint(GLFW_RESIZABLE, true);

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

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

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
		wnd->m_Events.push_back(WindowCloseEvent{});
		//wnd->m_WindowDescriptor.callbacks.onWindowClose();
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
		wnd->m_Events.push_back(WindowResizeEvent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) });
		//wnd->m_WindowDescriptor.callbacks.onViewportSizeChanged(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

}
