#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace SvgRenderer {

	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float near, float far)
		: m_ViewMatrix(glm::mat4(1.0f)), m_ProjectionMatrix(glm::ortho(left, right, bottom, top, near, far)), m_ViewProjectionMatrix(m_ProjectionMatrix * m_ViewMatrix)
	{
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top, float near, float far)
	{
		m_ProjectionMatrix = glm::ortho(left, right, bottom, top, near, far);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		glm::mat4 cameraModelMatrix = glm::translate(glm::mat4(1.0f), m_Position);
		m_ViewMatrix = glm::inverse(cameraModelMatrix);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}
