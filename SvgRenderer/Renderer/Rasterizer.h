#pragma once

#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	constexpr float TOLERANCE = 0.1f;

	struct Increment
	{
		int32_t x;
		int32_t y;
		float area;
		float height;
	};

	struct Tile
	{
		int32_t winding = 0;
	};

	struct Bin
	{
		const int32_t tileX;
		const int32_t tileY;
		size_t start;
		size_t end;
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

			tiles.resize(tileCount);

			bins.reserve(tileCount);
			for (int32_t y = 0; y < m_TileCountY; ++y)
			{
				for (int32_t x = 0; x < m_TileCountX; ++x)
				{
					bins.push_back(Bin{
						.tileX = x,
						.tileY = y,
						.start = 0,
						.end = 0
					});
				}
			}
		}

		Tile& GetTile(int x, int y) { return tiles[y * m_TileCountX + x]; }
		Bin& GetBin(int x, int y) { return bins[y * m_TileCountX + x]; }

		size_t GetTileIndex(int x, int y) const { return y * m_TileCountX + x; }

		uint32_t m_Width, m_Height;
		uint32_t m_TileCountX, m_TileCountY;

		std::vector<Increment> increments;
		std::vector<Tile> tiles;
		std::vector<Bin> bins;

		glm::vec2 first = { 0.0f, 0.0f };
		glm::vec2 last = { 0.0f, 0.0f };
		int16_t prevTileY = 0;

		void MoveTo(const glm::vec2& point);
		void LineTo(const glm::vec2& point);

		void Command(const PathCmd& command);

		void Fill(const std::vector<PathCmd>& path, const glm::mat3& transform);
		void Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform);

		void Finish(TileBuilder& builder);
	};

}
