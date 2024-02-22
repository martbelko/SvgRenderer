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

	glm::vec2 CubicBezier::Evaluate(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t)
	{
		const glm::vec2 v0 = (1.0f - t) * (1.0f - t) * (1.0f - t) * p0;
		const glm::vec2 v1 = 3.0f * (1.0f - t) * (1.0f - t) * t * p1;
		const glm::vec2 v2 = 3.0f * (1.0f - t) * t * t * p2;
		const glm::vec2 v3 = t * t * t * p3;
		return v0 + v1 + v2 + v3;
	}

	glm::vec2 CubicBezier::EvaluateDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t)
	{
		const glm::vec2 v0 = 3.0f * (1.0f - t) * (1.0f - t) * (p1 - p0);
		const glm::vec2 v1 = 6.0f * (1.0f - t) * t * (p2 - p1);
		const glm::vec2 v2 = 3.0f * t * t * (p3 - p2);
		return v0 + v1 + v2;
	}

	glm::vec2 CubicBezier::EvaluateSecondDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t)
	{
		const glm::vec2 v0 = 6.0f * (1.0f - t) * (p2 - 2.0f * p1 + p0);
		const glm::vec2 v1 = 6.0f * t * (p3 - 2.0f * p2 + p1);
		return v0 + v1;
	}

	BezierPoint CubicBezier::GetClosestPointToControlPoint(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, uint8_t index)
	{
		const glm::vec2& cp = index == 0 ? p1 : p2;

		auto fn = [&](float t) -> float
		{
			glm::vec2 v0 = CubicBezier::Evaluate(p0, p1, p2, p3, t) - cp;
			glm::vec2 v1 = CubicBezier::EvaluateDerivative(p0, p1, p2, p3, t);
			return glm::dot(v0, v1);
		};

		auto fnDerivative = [&](float t) -> float
		{
			glm::vec2 v0 = CubicBezier::EvaluateDerivative(p0, p1, p2, p3, t);
			glm::vec2 v1 = CubicBezier::Evaluate(p0, p1, p2, p3, t) - cp;
			glm::vec2 v2 = CubicBezier::EvaluateSecondDerivative(p0, p1, p2, p3, t);
			return glm::dot(v0, v0) + glm::dot(v1, v2);
		};

		float root = Equations::SolveEquationNewton(fn, fnDerivative, 0.5f);
		return BezierPoint{
			.t = root,
			.point = Evaluate(p0, p1, p2, p3, root)
		};
	}

}
