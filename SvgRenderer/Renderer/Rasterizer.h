#pragma once

#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include <glm/glm.hpp>

#include <array>

namespace SvgRenderer {

	class PathRenderCmd;

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
		Rasterizer(uint32_t windowWidth, uint32_t windowHeight);

		Tile& GetTile(int x, int y) { return tiles[y * m_TileCountX + x]; }

		size_t GetTileIndex(int x, int y) const { return y * m_TileCountX + x; }

		uint32_t m_Width, m_Height;
		uint32_t m_TileCountX, m_TileCountY;

		std::vector<Tile> tiles;

		glm::vec2 first = { 0.0f, 0.0f };
		glm::vec2 last = { 0.0f, 0.0f };
		int16_t prevTileY = 0;

		void MoveTo(const glm::vec2& point);
		void LineTo(const glm::vec2& p1);

		void Command(const PathRenderCmd& command, const glm::vec2& lastPoint);
		void Command(const PathCmd& command, const glm::vec2& lastPoint);

		void Fill(uint32_t pathIndex);
		void Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform);

		void Finish(TileBuilder& builder);
	};

}
