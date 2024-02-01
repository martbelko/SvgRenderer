#pragma once

#include <glm/glm.hpp>

#include <limits>

namespace SvgRenderer {

	struct BoundingBox
	{
		glm::vec2 min = glm::vec2(std::numeric_limits<float>::max());
		glm::vec2 max = glm::vec2(-std::numeric_limits<float>::max());
	};

}
