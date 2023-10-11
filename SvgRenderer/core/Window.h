typedef struct GLFWwindow GLFWwindow;

namespace SvgRenderer {

	class Window
	{
	public:
		Window(uint32_t width = 1280, uint32_t height = 960, const std::string& title = "Svg Renderer");
		~Window();

		void OnUpdate();

		std::pair<uint32_t, uint32_t> GetWindowSize() const
		{
			return std::make_pair(m_Width, m_Height);
		}
	private:
		uint32_t m_Width, m_Height;
		GLFWwindow* m_NativeWindow;
	};

}
