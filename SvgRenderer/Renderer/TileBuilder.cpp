#include "TileBuilder.h"

#include "Core/Base.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	TileBuilder::TileBuilder()
	{
		atlas.resize(ATLAS_SIZE * ATLAS_SIZE, 0);
		atlas[0] = 1.0f;
	}

	void TileBuilder::Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data, uint32_t tileOffset, uint32_t quadIndex, const std::array<uint8_t, 4>& color)
	{
		size_t base = quadIndex * 4;

		tileOffset += 1;
		uint32_t col = tileOffset % (ATLAS_SIZE / TILE_SIZE);
		uint32_t row = tileOffset / (ATLAS_SIZE / TILE_SIZE);
		uint32_t u1 = col * TILE_SIZE;
		uint32_t u2 = (col + 1) * TILE_SIZE;
		uint32_t v1 = row * TILE_SIZE;
		uint32_t v2 = (row + 1) * TILE_SIZE;

		vertices[base + 0] = Vertex{
			.pos = { x, y },
			.uv = { u1, v1 },
			.color = color,
			};
		vertices[base + 1] = Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + TILE_SIZE)), y },
			.uv = { u2, v1 },
			.color = color,
			};
		vertices[base + 2] = Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + TILE_SIZE)), static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { u2, v2 },
			.color = color,
			};
		vertices[base + 3] = Vertex{
			.pos = { x, static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { u1, v2 },
			.color = color,
			};

		for (uint32_t y = 0; y < TILE_SIZE; ++y)
		{
			for (uint32_t x = 0; x < TILE_SIZE; ++x)
			{
				size_t index = row * TILE_SIZE * ATLAS_SIZE
					+ y * ATLAS_SIZE
					+ col * TILE_SIZE
					+ x;
				atlas[index] = data[y * TILE_SIZE + x] / 255.0f;
			}
		}
	}

	void TileBuilder::Span(int32_t x, int32_t y, uint32_t width, uint32_t quadIndex, const std::array<uint8_t, 4>& color)
	{
		uint32_t base = quadIndex * 4;

		vertices[base + 0] = Vertex{
			.pos = { x, y },
			.uv = { 0, 0 },
			.color = color,
			};
		vertices[base + 1] = Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + width)), y },
			.uv = { 0, 0 },
			.color = color,
			};
		vertices[base + 2] = Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + width)), static_cast<int32_t>(glm::floor(y + TILE_SIZE)) } ,
			.uv = { 0, 0 },
			.color = color,
			};
		vertices[base + 3] = Vertex{
			.pos = { x, static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { 0, 0 },
			.color = color,
			};
	}

}
