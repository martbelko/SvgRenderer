#pragma once

#include "Utils/BoundingBox.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	struct PathRender
	{
		uint32_t startCmdIndex;
		uint32_t endCmdIndex;
		glm::mat3 transform;
		std::array<uint8_t, 4> color;
		BoundingBox bbox;
	};

	struct PathRenderCmd
	{
		uint32_t pathIndexCmdType; // 16 bits pathIndex, 8 bits curve type, 8 bits unused, GET_CMD_PATH_INDEX, GET_CMD_TYPE, MAKE_CMD_PATH_INDEX, MAKE_CMD_TYPE
		std::array<glm::vec2, 3> points; // Maybe unused, but maximum 3 points for cubicTo
		std::array<glm::vec2, 3> transformedPoints; // Maybe unused, but maximum 3 points for cubicTo
	};

	struct PathsContainer
	{
		std::vector<PathRender> paths;
		std::vector<PathRenderCmd> commands;
		std::vector<uint32_t> numOfSegments; // Specific for each path, size of paths + 1, first number will be zero
		std::vector<uint32_t> positions;
	};

	class Globals
	{
	public:
		inline static glm::mat4 GlobalTransform = glm::mat4(1.0f);
		inline static PathsContainer AllPaths;
	};

}
