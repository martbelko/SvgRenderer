#pragma once

#include "Renderer/Defs.h"

#include <array>
#include <vector>

namespace SvgRenderer {

	struct Vertex
	{
		std::array<int32_t, 2> pos;
		std::array<uint32_t, 2> uv;
		std::array<uint8_t, 4> color;
	};

	class TileBuilder
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint8_t> atlas;
		std::array<uint8_t, 4> color = { 255, 255, 255, 255 };

		TileBuilder();

		void Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data, uint32_t tileOffset);
		void Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data, uint32_t tileOffset, uint32_t quadIndex);
		void Span(int32_t x, int32_t y, uint32_t width);
		void Span(int32_t x, int32_t y, uint32_t width, uint32_t quadIndex);
	};

}
