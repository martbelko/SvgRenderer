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

		void AddPadding(const glm::vec2& padding)
		{
			min.x -= padding.x;
			max.x += padding.x;
			min.y -= padding.y;
			max.y += padding.y;
		}

		static BoundingBox Merge(const BoundingBox& bbox1, const BoundingBox& bbox2)
		{
			return BoundingBox{
				.min = glm::min(bbox1.min, bbox2.min),
				.max = glm::max(bbox1.max, bbox2.max)
			};
		}
	};

}
