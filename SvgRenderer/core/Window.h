#pragma once

typedef struct GLFWwindow GLFWwindow;

namespace SvgRenderer {

	struct WindowCallbacks
	{
		using OnWindowCloseFn = void(*)();
		using OnKeyPressedFn = void(*)(int, int);
		using OnKeyReleasedFn = void(*)(int);
		using OnMousePressedFn = void(*)(int, int);
		using OnMouseReleasedFn = void(*)(int);
		using OnViewportSizeChangedFn = void(*)(uint32_t, uint32_t);

		OnWindowCloseFn onWindowClose = nullptr;
		OnKeyPressedFn onKeyPressed = nullptr;
		OnKeyReleasedFn onKeyReleased = nullptr;
		OnMousePressedFn onMousePressed = nullptr;
		OnMouseReleasedFn onMouseReleased = nullptr;
		OnViewportSizeChangedFn onViewportSizeChanged = nullptr;
	};

	struct WindowDesc
	{
		uint32_t width, height;
		std::string title;
		WindowCallbacks callbacks;
	};

	class Window
	{
	public:
		Window(const WindowDesc& desc);

		void OnUpdate();

		void Close();

		std::pair<uint32_t, uint32_t> GetWindowSize() const
		{
			return std::make_pair(m_WindowDescriptor.width, m_WindowDescriptor.height);
		}
	public:
		static Scope<Window> Create(const WindowDesc& desc)
		{
			return CreateScope<Window>(desc);
		}
	private:
		static void OnWindowShouldClose(GLFWwindow* window);
		static void OnKeyChanged(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void OnMouseChanged(GLFWwindow* window, int button, int action, int mods);
		static void OnViewportSizeChanged(GLFWwindow* window, int width, int height);
	private:
		WindowDesc m_WindowDescriptor;
		GLFWwindow* m_NativeWindow;
	};

}
