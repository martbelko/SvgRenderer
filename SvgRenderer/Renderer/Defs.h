#pragma once

#include "Utils/BoundingBox.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <execution>

namespace SvgRenderer {

	#define MOVE_TO 0
	#define LINE_TO 1
	#define QUAD_TO 2
	#define CUBIC_TO 3

	#define GET_CMD_PATH_INDEX(value) (value >> 16)
	#define GET_CMD_TYPE(value) ((value & 0x0000FF00) >> 8)
	#define MAKE_CMD_PATH_INDEX(value, index) ((index << 16) | (value & 0x0000FFFF))
	#define MAKE_CMD_TYPE(value, type) ((type << 8) | (value & 0xFFFF00FF))

#define ASYNC 2
#if ASYNC == 0
	static constexpr std::execution::sequenced_policy executionPolicy = std::execution::seq;
#elif ASYNC == 1
	static constexpr std::execution::parallel_policy executionPolicy = std::execution::par;
#elif ASYNC == 2 // GPU
	static constexpr std::execution::parallel_policy executionPolicy = std::execution::par;
#endif

	struct SimpleCommand // Lines or moves only
	{
		uint32_t type;
		glm::vec2 point;
	};

	struct PathRender
	{
		uint32_t startCmdIndex;
		uint32_t endCmdIndex;
		uint32_t startTileIndex;
		uint32_t endTileIndex;
		glm::mat4 transform;
		BoundingBox bbox;
		std::array<uint8_t, 4> color;
		uint32_t startVisibleTileIndex;
		uint32_t startSpanQuadIndex;
		uint32_t startTileQuadIndex;
	};

	struct PathRenderCmd
	{
		uint32_t pathIndexCmdType; // 16 bits pathIndex, 8 bits curve type, 8 bits unused, GET_CMD_PATH_INDEX, GET_CMD_TYPE, MAKE_CMD_PATH_INDEX, MAKE_CMD_TYPE
		uint32_t startIndexSimpleCommands;
		uint32_t endIndexSimpleCommands;
		uint32_t _pad0;
		std::array<glm::vec2, 4> points; // Maybe unused, but maximum 3 points for cubicTo
		std::array<glm::vec2, 4> transformedPoints; // Maybe unused, but maximum 3 points for cubicTo
	};

	struct PathsContainer
	{
		std::vector<PathRender> paths;
		std::vector<PathRenderCmd> commands;
		std::vector<SimpleCommand> simpleCommands;
	};

	struct Increment
	{
		float area = 0;
		float height = 0;
	};

	constexpr float TOLERANCE = 0.2f; // Quality of flattening
	constexpr int8_t TILE_SIZE = 16;
	constexpr uint32_t ATLAS_SIZE = 4096 * 2;

	constexpr uint32_t SCREEN_WIDTH = 1900;
	constexpr uint32_t SCREEN_HEIGHT = 1000;

	struct Tile
	{
		int32_t winding = 0;
		uint32_t nextTileIndex = std::numeric_limits<uint32_t>::max();
		bool hasIncrements = false;
		std::array<Increment, TILE_SIZE * TILE_SIZE> increments;
	};

	struct TilesContainer
	{
		std::vector<Tile> tiles;
	};

	class Globals
	{
	public:
		inline static glm::mat4 GlobalTransform = glm::mat4(1.0f); //glm::translate(glm::mat4(1.0f), glm::vec3(-800, 0, 0))* glm::scale(glm::mat4(1.0f), { 3.0f, 3.0f, 1.0f });
		inline static PathsContainer AllPaths;
		inline static TilesContainer Tiles;
	};

}
