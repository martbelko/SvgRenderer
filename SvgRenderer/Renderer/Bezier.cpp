#include "Bezier.h"

#include "Utils/Equations.h"

#include <cassert>

namespace SvgRenderer {

	glm::vec2 QuadraticBezier::Evaluate(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t)
	{
		const glm::vec2 v0 = (1.0f - t) * (1.0f - t) * p0;
		const glm::vec2 v1 = 2.0f * t * (1.0f - t) * p1;
		const glm::vec2 v2 = t * t * p2;
		return v0 + v1 + v2;
	}

	glm::vec2 QuadraticBezier::EvaluateDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t)
	{
		return 2.0f * (t * (p2 - 2.0f * p1 + p0) + p2 - p1);
	}

	BezierPoint QuadraticBezier::GetClosestPointToControlPoint(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2)
	{
		const glm::vec2 v0 = p1 - p0;
		const glm::vec2 v1 = p2 - p1;

		const float a = glm::dot(v1 - v0, v1 - v0);
		const float b = 3.0f * (glm::dot(v1, v0) - glm::dot(v0, v0));
		const float c = 3.0f * glm::dot(v0, v0) - glm::dot(v1, v0);
		const float d = -glm::dot(v0, v0);

		Equations::CubicRoot roots = Equations::SolveCubic(a, b, c, d);
		float root = roots.x1;
		root = glm::abs(root - 0.5f) < glm::abs(roots.x2 - 0.5f) ? root : roots.x2;
		root = glm::abs(root - 0.5f) < glm::abs(roots.x3 - 0.5f) ? root : roots.x3;

		assert(root >= 0.0f && root <= 1.0f);
		return BezierPoint{
			.t = root,
			.point = Evaluate(p0, p1, p2, root)
		};
	}

}
