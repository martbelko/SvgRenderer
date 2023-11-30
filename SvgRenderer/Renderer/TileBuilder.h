#pragma once

#include <array>
#include <vector>

namespace SvgRenderer {

	constexpr uint32_t TILE_SIZE = 8;
	constexpr uint32_t ATLAS_SIZE = 4096;

	struct Vertex
	{
		std::array<int16_t, 2> pos;
		std::array<uint16_t, 2> uv;
		std::array<uint8_t, 4> color;
	};

	class TileBuilder
	{
	public:
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint8_t> atlas;
		uint16_t nextRow = 0;
		uint16_t nextCol = 1;
		std::array<uint8_t, 4> color = { 255, 255, 255, 255 };

		TileBuilder();

		void Tile(int16_t x, int16_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data);
		void Span(int16_t x, int16_t y, uint16_t width);
	};

}
