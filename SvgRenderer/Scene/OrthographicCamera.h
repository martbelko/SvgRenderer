#pragma once

#include <glm/glm.hpp>

namespace SvgRenderer {

	class OrthographicCamera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top, float near = -1.0f, float far = 1.0f);

		void SetProjection(float left, float right, float bottom, float top, float near = -1.0f, float far = 1.0f);

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	private:
		void RecalculateViewMatrix();
	private:
		glm::vec3 m_Position = glm::vec3(0, 0, 0);
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;
	};

}
