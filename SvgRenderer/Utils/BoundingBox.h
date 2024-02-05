#pragma once

#include <glm/glm.hpp>

#include <limits>

namespace SvgRenderer {

	struct BoundingBox
	{
		glm::vec2 min = glm::vec2(std::numeric_limits<float>::max());
		glm::vec2 max = glm::vec2(-std::numeric_limits<float>::max());

		void AddPoint(const glm::vec2& point)
		{
			min = glm::min(min, point);
			max = glm::max(max, point);
		}
	};

}
