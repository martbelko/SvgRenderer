#pragma once

#include "core/Window.h"

#include "Renderer/TileBuilder.h"
#include "Renderer/Rasterizer.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	struct CurvedPolygon;
	class Shader;

	class Application
	{
	public:
		void Init();
		void Shutdown();

		void Run();
	public:
		static Application& Get() { return s_Instance; }
	public:
		static void OnWindowCloseStatic() { Get().OnWindowClose(); }

		static void OnKeyPressedStatic(int key, int repeat) { Get().OnKeyPressed(key, repeat); }
		static void OnKeyReleasedStatic(int key) { Get().OnKeyReleased(key); }

		static void OnMousePressedStatic(int button, int repeat) { Get().OnMousePressed(button, repeat); }
		static void OnMouseReleasedStatic(int button) { Get().OnMouseReleased(button); }

		static void OnViewportResizeStatic(uint32_t width, uint32_t height) { Get().OnViewportResize(width, height); }
	private:
		Application() = default;

		void OnWindowClose();

		void OnKeyPressed(int key, int repeat);
		void OnKeyReleased(int key);

		void OnMousePressed(int button, int repeat);
		void OnMouseReleased(int button);

		void OnViewportResize(uint32_t width, uint32_t height);
	private:
		bool m_Running = false;
		Scope<Window> m_Window;

		TileBuilder m_TileBuilder;

		uint32_t m_Vbo, m_Ibo, m_Vao, m_AlphaTexture;
		Ref<Shader> m_FinalShader;
	private:
		static Application s_Instance;
	};

}
