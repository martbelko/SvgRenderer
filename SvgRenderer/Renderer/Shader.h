#pragma once

namespace SvgRenderer {

	class Shader
	{
	public:
		Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
		~Shader();

		void Bind() const;

		uint32_t GetRendererId() const { return m_RendererId; }
	public:
		static Ref<Shader> Create(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
		{
			return CreateRef<Shader>(vertexPath, fragmentPath);
		}
	private:
		static std::string ReadFile(const std::filesystem::path& filepath);
		static uint32_t CompileShader(const std::string& source, uint32_t shaderType);
		static uint32_t LinkShader(uint32_t vertexShader, uint32_t fragmentShader);
	private:
		uint32_t m_RendererId;
	};

}
