#pragma once

#include <array>
#include <vector>

namespace SvgRenderer {

	constexpr uint32_t TILE_SIZE = 8;
	constexpr uint32_t ATLAS_SIZE = 4096 * 2;

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
		uint32_t nextRow = 0;
		uint32_t nextCol = 1;
		std::array<uint8_t, 4> color = { 255, 255, 255, 255 };

		TileBuilder();

		void Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data);
		void Span(int32_t x, int32_t y, uint32_t width);
	};

}
