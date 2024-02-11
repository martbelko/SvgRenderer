#pragma once

#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include "Utils/BoundingBox.h"

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
		Rasterizer(const BoundingBox& bbox);

		Tile& GetTileFromRelativePos(int32_t x, int32_t y) { return tiles[GetTileIndexFromRelativePos(x, y)]; }
		Tile& GetTileFromWindowPos(int32_t x, int32_t y) { return tiles[GetTileIndexFromWindowPos(x, y)]; }

		uint32_t GetTileIndexFromRelativePos(int32_t x, int32_t y) const { return y * m_TileCountX + x; }
		uint32_t GetTileIndexFromWindowPos(int32_t x, int32_t y) const
		{
			int32_t offsetX = x / TILE_SIZE - m_TileStartX;
			int32_t offsetY = y / TILE_SIZE - m_TileStartY;
			return offsetY * m_TileCountX + offsetX;
		}

		uint32_t GetTileCoordX(int32_t windowPosX) const { return windowPosX / TILE_SIZE - m_TileStartX; }
		uint32_t GetTileCoordY(int32_t windowPosY) const { return windowPosY / TILE_SIZE - m_TileStartY; }

		uint32_t m_Width, m_Height;

		int32_t m_TileStartX, m_TileStartY;
		uint32_t m_TileCountX, m_TileCountY;

		std::vector<Tile> tiles;

		glm::vec2 first = { 0.0f, 0.0f };
		glm::vec2 last = { 0.0f, 0.0f };
		int16_t prevTileY = 0;

		void MoveTo(const glm::vec2& point);
		void LineTo(const glm::vec2& p1);

		void CommandFromArray(const PathRenderCmd& command, const glm::vec2& lastPoint);
		void FillFromArray(uint32_t pathIndex);
		void Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform);

		void Finish(TileBuilder& builder);
	};

}
