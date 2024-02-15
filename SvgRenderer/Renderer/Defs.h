#pragma once

#include "Utils/BoundingBox.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SvgRenderer {

	#define MOVE_TO 0
	#define LINE_TO 1
	#define QUAD_TO 2
	#define CUBIC_TO 3

	#define GET_CMD_PATH_INDEX(value) (value >> 16)
	#define GET_CMD_TYPE(value) ((value & 0x0000FF00) >> 8)
	#define MAKE_CMD_PATH_INDEX(value, index) ((index << 16) | (value & 0x0000FFFF))
	#define MAKE_CMD_TYPE(value, type) ((type << 8) | (value & 0xFFFF00FF))

	struct SimpleCommand // Lines or moves only
	{
		uint32_t type;
		glm::vec2 point;
	};

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
		uint32_t startIndexSimpleCommands;
		uint32_t endIndexSimpleCommands;
		std::array<glm::vec2, 3> points; // Maybe unused, but maximum 3 points for cubicTo
		std::array<glm::vec2, 3> transformedPoints; // Maybe unused, but maximum 3 points for cubicTo
	};

	struct PathsContainer
	{
		std::vector<PathRender> paths;
		std::vector<PathRenderCmd> commands;
		std::vector<SimpleCommand> simpleCommands;
	};

	class Globals
	{
	public:
		inline static glm::mat4 GlobalTransform = glm::mat4(1.0f); // glm::translate(glm::mat4(1.0f), glm::vec3(200, 100, 0));
		inline static PathsContainer AllPaths;
	};

}
