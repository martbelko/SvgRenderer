#pragma once

#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include <glm/glm.hpp>

#include <array>

namespace SvgRenderer {

	constexpr float TOLERANCE = 0.1f;

	struct Increment
	{
		int32_t x = 0;
		int32_t y = 0;
		float area = 0;
		float height = 0;
	};

	struct Tile
	{
		int32_t tileX = 0, tileY = 0;
		int32_t winding = 0;
		bool hasIncrements = false;
		std::array<Increment, TILE_SIZE * TILE_SIZE> increments;
	};

	class Rasterizer
	{
	public:
		Rasterizer(uint32_t windowWidth, uint32_t windowHeight)
			: m_Width(windowWidth), m_Height(windowHeight)
		{
			m_TileCountX = std::ceil(static_cast<float>(m_Width) / TILE_SIZE);
			m_TileCountY = std::ceil(static_cast<float>(m_Height) / TILE_SIZE);
			uint32_t tileCount = m_TileCountX * m_TileCountY;

			tiles.reserve(tileCount);
			for (int32_t y = 0; y < m_TileCountY; ++y)
			{
				for (int32_t x = 0; x < m_TileCountX; ++x)
				{
					tiles.push_back(Tile{
						.tileX = x,
						.tileY = y,
						.winding = 0,
						.hasIncrements = false,
						.increments = std::array<Increment, TILE_SIZE * TILE_SIZE>()
						});

					auto& t = tiles.back();
					for (int32_t relativeY = 0; relativeY < TILE_SIZE; relativeY++)
					{
						for (int32_t relativeX = 0; relativeX < TILE_SIZE; relativeX++)
						{
							t.increments[relativeY * TILE_SIZE + relativeX].x = relativeX;
							t.increments[relativeY * TILE_SIZE + relativeX].y = relativeY;
							t.increments[relativeY * TILE_SIZE + relativeX].area = 0;
							t.increments[relativeY * TILE_SIZE + relativeX].height = 0;
						}
					}
				}
			}
		}

		Tile& GetTile(int x, int y) { return tiles[y * m_TileCountX + x]; }

		size_t GetTileIndex(int x, int y) const { return y * m_TileCountX + x; }

		uint32_t m_Width, m_Height;
		uint32_t m_TileCountX, m_TileCountY;

		std::vector<Tile> tiles;

		glm::vec2 first = { 0.0f, 0.0f };
		glm::vec2 last = { 0.0f, 0.0f };
		int16_t prevTileY = 0;

		void MoveTo(const glm::vec2& point);
		void LineTo(const glm::vec2& point);

		void Command(const PathCmd& command);

		void Fill(const std::vector<PathCmd>& path);
		void Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform);

		void Finish(TileBuilder& builder);
	};

}
