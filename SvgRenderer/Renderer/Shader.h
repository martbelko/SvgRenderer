#pragma once

#include <glm/glm.hpp>

namespace SvgRenderer {

	class Shader
	{
	public:
		Shader(uint32_t rendererId)
			: m_RendererId(rendererId) {}
		Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
		~Shader();

		void Dispatch(uint32_t x, uint32_t y, uint32_t z);

		void Bind() const;

		uint32_t GetRendererId() const { return m_RendererId; }

		void SetUniformInt(uint32_t uniformLocation, int value);
		void SetUniformIntArray(uint32_t uniformLocation, int* values, uint32_t count);
		void SetUniformFloat(uint32_t uniformLocation, float value);
		void SetUniformFloat2(uint32_t uniformLocation, const glm::vec2& value);
		void SetUniformFloat3(uint32_t uniformLocation, const glm::vec3& value);
		void SetUniformFloat4(uint32_t uniformLocation, const glm::vec4& value);
		void SetUniformMat3(uint32_t uniformLocation, const glm::mat3& matrix);
		void SetUniformMat4(uint32_t uniformLocation, const glm::mat4& matrix);
	public:
		static Ref<Shader> Create(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
		{
			return CreateRef<Shader>(vertexPath, fragmentPath);
		}

		static Ref<Shader> CreateCompute(const std::filesystem::path& filepath);
	private:
		static std::string ReadFile(const std::filesystem::path& filepath);
		static uint32_t CompileShader(const std::string& source, uint32_t shaderType);
		static uint32_t LinkShader(uint32_t vertexShader, uint32_t fragmentShader);
		static uint32_t LinkShader(uint32_t computeShader);
	private:
		uint32_t m_RendererId;
	};

}
