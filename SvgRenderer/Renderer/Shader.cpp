#include "Shader.h"

#include <glad/glad.h>

#include <glm/gtc/type_ptr.hpp>

namespace SvgRenderer {

	Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	{
		std::string source = ReadFile(vertexPath);
		uint32_t vertexShader = CompileShader(source, GL_VERTEX_SHADER);

		source = ReadFile(fragmentPath);
		uint32_t fragmentShader = CompileShader(source, GL_FRAGMENT_SHADER);

		m_RendererId = LinkShader(vertexShader, fragmentShader);
	}

	Shader::~Shader()
	{
		glDeleteProgram(m_RendererId);
	}

	void Shader::Dispatch(uint32_t x, uint32_t y, uint32_t z)
	{
		glDispatchCompute(x, y, z);
	}

	void Shader::Bind() const
	{
		glUseProgram(m_RendererId);
	}

	std::string Shader::ReadFile(const std::filesystem::path& filepath)
	{
		std::string buffer;
		std::ifstream f(filepath);
		f.seekg(0, std::ios::end);
		buffer.resize(f.tellg());
		f.seekg(0);
		f.read(buffer.data(), buffer.size());

		return buffer;
	}

	uint32_t Shader::CompileShader(const std::string& source, uint32_t shaderType)
	{
		uint32_t shader = glCreateShader(shaderType);
		const char* sourcePtr = source.c_str();

		glShaderSource(shader, 1, &sourcePtr, NULL);
		glCompileShader(shader);

		int success;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(shader, 512, NULL, infoLog);
			SR_ERROR("Error with shader compilation:\n\t{0}", infoLog);
			return 0;
		}

		return shader;
	}

	uint32_t Shader::LinkShader(uint32_t vertexShader, uint32_t fragmentShader)
	{
		uint32_t program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		int success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetProgramInfoLog(program, 512, NULL, infoLog);
			SR_ERROR("Error with shader linking:\n\t{0}", infoLog);
			return 0;
		}

		glDetachShader(program, vertexShader);
		glDetachShader(program, fragmentShader);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		return program;
	}

	uint32_t Shader::LinkShader(uint32_t computeShader)
	{
		uint32_t program = glCreateProgram();
		glAttachShader(program, computeShader);
		glLinkProgram(program);

		int success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char infoLog[512];
			glGetProgramInfoLog(program, 512, NULL, infoLog);
			SR_ERROR("Error with shader linking:\n\t{0}", infoLog);
			return 0;
		}

		glDetachShader(program, computeShader);
		glDeleteShader(computeShader);

		return program;
	}

	Ref<Shader> Shader::CreateCompute(const std::filesystem::path& filepath)
	{
		std::string source = ReadFile(filepath);
		uint32_t shader = CompileShader(source, GL_COMPUTE_SHADER);

		uint32_t rendererId = LinkShader(shader);
		return CreateRef<Shader>(rendererId);
	}

	void Shader::SetUniformInt(uint32_t uniformLocation, int value)
	{
		glUniform1i(uniformLocation, value);
	}

	void Shader::SetUniformIntArray(uint32_t uniformLocation, int* values, uint32_t count)
	{
		glUniform1iv(uniformLocation, count, values);
	}

	void Shader::SetUniformFloat(uint32_t uniformLocation, float value)
	{
		glUniform1f(uniformLocation, value);
	}

	void Shader::SetUniformFloat2(uint32_t uniformLocation, const glm::vec2& value)
	{
		glUniform2f(uniformLocation, value.x, value.y);
	}

	void Shader::SetUniformFloat3(uint32_t uniformLocation, const glm::vec3& value)
	{
		glUniform3f(uniformLocation, value.x, value.y, value.z);
	}

	void Shader::SetUniformFloat4(uint32_t uniformLocation, const glm::vec4& value)
	{
		glUniform4f(uniformLocation, value.x, value.y, value.z, value.w);
	}

	void Shader::SetUniformMat3(uint32_t uniformLocation, const glm::mat3& matrix)
	{
		glUniformMatrix3fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void Shader::SetUniformMat4(uint32_t uniformLocation, const glm::mat4& matrix)
	{
		glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, glm::value_ptr(matrix));
	}

}
