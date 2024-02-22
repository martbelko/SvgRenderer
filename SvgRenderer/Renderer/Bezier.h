#pragma once

#include <glm/glm.hpp>

namespace SvgRenderer {

	struct BezierPoint
	{
		float t;
		glm::vec2 point;
	};

	struct QuadraticBezier
	{
		glm::vec2 p0;
		glm::vec2 p1;
		glm::vec2 p2;

		glm::vec2 Evaluate(float t) const { return Evaluate(p0, p1, p2, t); }
		glm::vec2 EvaluateDerivative(float t) const { return EvaluateDerivative(p0, p1, p2, t); }
		BezierPoint GetClosestPointToControlPoint() const { return GetClosestPointToControlPoint(p0, p1, p2); }

		static glm::vec2 Evaluate(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t);
		static glm::vec2 EvaluateDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t);
		static BezierPoint GetClosestPointToControlPoint(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2);
	};

	struct CubicBezier
	{
		glm::vec2 p0;
		glm::vec2 p1;
		glm::vec2 p2;
		glm::vec2 p3;

		glm::vec2 Evaluate(float t) const { return Evaluate(p0, p1, p2, p3, t); }
		glm::vec2 EvaluateDerivative(float t) const { return EvaluateDerivative(p0, p1, p2, p3, t); }
		glm::vec2 EvaluateSecondDerivative(float t) const { return EvaluateSecondDerivative(p0, p1, p2, p3, t); }

		// Index may be 0 or 1 (0 - p1, 1 - p2)
		BezierPoint GetClosestPointToControlPoint(uint8_t index) const { return GetClosestPointToControlPoint(p0, p1, p2, p3, index); }

		static glm::vec2 Evaluate(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t);
		static glm::vec2 EvaluateDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t);
		static glm::vec2 EvaluateSecondDerivative(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, float t);

		// Index may be 0 or 1 (0 - p1, 1 - p2)
		static BezierPoint GetClosestPointToControlPoint(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& p3, uint8_t index);
	};

}
