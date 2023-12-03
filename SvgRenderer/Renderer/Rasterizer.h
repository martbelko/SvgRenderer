#pragma once

#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	constexpr float TOLERANCE = 0.1f;

	struct Increment
	{
		int16_t x;
		int16_t y;
		float area;
		float height;
	};

	struct Tile
	{
		int32_t winding = 0;
	};

	struct TileIncrement
	{
		int16_t tileX;
		int16_t tileY;
		int8_t sign;
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
		}

		Tile& GetTile(int x, int y) { return tiles[y * m_TileCountX + x]; }
		size_t GetTileIndex(int x, int y) const { return y * m_TileCountX + x; }

		uint32_t m_Width, m_Height;
		uint32_t m_TileCountX, m_TileCountY;

		std::vector<Increment> increments;
		std::vector<Tile> tiles;
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
