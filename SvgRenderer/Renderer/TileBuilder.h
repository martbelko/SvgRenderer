#pragma once

#include "Renderer/Defs.h"

#include <array>
#include <vector>

namespace SvgRenderer {

	struct Vertex
	{
		glm::vec<2, int32_t> pos;
		glm::vec<2, uint32_t> uv;
		glm::vec<4, uint8_t> color;
	};

	class TileBuilder
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<float> atlas;

		void Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data, uint32_t tileOffset, uint32_t quadIndex, const std::array<uint8_t, 4>& color);
		void Span(int32_t x, int32_t y, uint32_t width, uint32_t quadIndex, const std::array<uint8_t, 4>& color);
	};

}
