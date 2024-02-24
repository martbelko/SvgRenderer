#include "TileBuilder.h"

#include "Core/Base.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	TileBuilder::TileBuilder()
	{
		atlas.resize(ATLAS_SIZE * ATLAS_SIZE, 0);
		for (uint32_t row = 0; row < TILE_SIZE; ++row)
		{
			for (uint32_t col = 0; col < TILE_SIZE; ++col)
			{
				atlas[row * ATLAS_SIZE + col] = 255;
			}
		}
	}

	void TileBuilder::Tile(int32_t x, int32_t y, const std::array<uint8_t, TILE_SIZE * TILE_SIZE>& data)
	{
		size_t base = vertices.size();

		uint32_t u1 = nextCol * TILE_SIZE;
		uint32_t u2 = (nextCol + 1) * TILE_SIZE;
		uint32_t v1 = nextRow * TILE_SIZE;
		uint32_t v2 = (nextRow + 1) * TILE_SIZE;

		vertices.push_back(Vertex{
			.pos = { x, y },
			.uv = { u1, v1 },
			.color = color,
		});
		vertices.push_back(Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + TILE_SIZE)), y },
			.uv = { u2, v1 },
			.color = color,
			});
		vertices.push_back(Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + TILE_SIZE)), static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { u2, v2 },
			.color = color,
			});
		vertices.push_back(Vertex{
			.pos = { x, static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { u1, v2 },
			.color = color,
			});

		indices.push_back(base);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 3);

		for (uint32_t row = 0; row < TILE_SIZE; ++row)
		{
			for (uint32_t col = 0; col < TILE_SIZE; ++col)
			{
				size_t index = nextRow * TILE_SIZE * ATLAS_SIZE
					+ row * ATLAS_SIZE
					+ nextCol * TILE_SIZE
					+ col;
				atlas[index] = data[row * TILE_SIZE + col];
			}
		}

		nextCol += 1;
		if (nextCol == ATLAS_SIZE / TILE_SIZE)
		{
			nextCol = 0;
			nextRow += 1;
		}
	}

	void TileBuilder::Span(int32_t x, int32_t y, uint32_t width)
	{
		size_t base = vertices.size();

		vertices.push_back(Vertex{
			.pos = { x, y },
			.uv = { 0, 0 },
			.color = color,
		});
		vertices.push_back(Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + width)), y },
			.uv = { 0, 0 },
			.color = color,
		});
		vertices.push_back(Vertex{
			.pos = { static_cast<int32_t>(glm::floor(x + width)), static_cast<int32_t>(glm::floor(y + TILE_SIZE)) } ,
			.uv = { 0, 0 },
			.color = color,
		});
		vertices.push_back(Vertex{
			.pos = { x, static_cast<int32_t>(glm::floor(y + TILE_SIZE)) },
			.uv = { 0, 0 },
			.color = color,
		});

		indices.push_back(base);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
	}

}
