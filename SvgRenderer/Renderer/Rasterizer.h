#pragma once

#include "Renderer/Defs.h"
#include "Renderer/TileBuilder.h"
#include "Renderer/Path.h"

#include "Utils/BoundingBox.h"

#include <glm/glm.hpp>

#include <array>

namespace SvgRenderer {

	class PathRenderCmd;

	class Rasterizer
	{
	public:
		Rasterizer(const BoundingBox& bbox, uint32_t pathIndex);

		Tile& GetTileFromRelativePos(int32_t x, int32_t y) { return Globals::Tiles.tiles[Globals::AllPaths.paths[pathIndex].startTileIndex + GetTileIndexFromRelativePos(x, y)]; }
		Tile& GetTileFromWindowPos(int32_t x, int32_t y) { return Globals::Tiles.tiles[Globals::AllPaths.paths[pathIndex].startTileIndex + GetTileIndexFromWindowPos(x, y)]; }

		uint32_t GetTileIndexFromRelativePos(int32_t x, int32_t y) const { return y * m_TileCountX + x; }
		uint32_t GetTileIndexFromWindowPos(int32_t x, int32_t y) const
		{
			int32_t offsetX = glm::floor(static_cast<float>(x) / TILE_SIZE) - m_TileStartX;
			int32_t offsetY = glm::floor(static_cast<float>(y) / TILE_SIZE) - m_TileStartY;
			return offsetY * m_TileCountX + offsetX;
		}

		uint32_t GetTileCoordX(int32_t windowPosX) const { return glm::floor(static_cast<float>(windowPosX) / TILE_SIZE) - m_TileStartX; }
		uint32_t GetTileCoordY(int32_t windowPosY) const { return glm::floor(static_cast<float>(windowPosY) / TILE_SIZE) - m_TileStartY; }

		int32_t GetTileXFromAbsoluteIndex(uint32_t absIndex) const
		{
			int32_t offset = absIndex % m_TileCountX;
			return m_TileStartX + offset;
		}

		int32_t GetTileYFromAbsoluteIndex(uint32_t absIndex) const
		{
			int32_t offset = absIndex / m_TileCountX;
			return m_TileStartY + offset;
		}

		int32_t m_TileStartX, m_TileStartY;
		uint32_t m_TileCountX, m_TileCountY;

		uint32_t pathIndex;

		glm::vec2 first = { 0.0f, 0.0f };
		glm::vec2 last = { 0.0f, 0.0f };
		int16_t prevTileY = 0;

		void MoveTo(const glm::vec2& point);
		void LineTo(const glm::vec2& p1);

		void CommandFromArray(const PathRenderCmd& command, const glm::vec2& lastPoint);
		void FillFromArray(uint32_t pathIndex);

		void Coarse(TileBuilder& builder);
		void Finish(TileBuilder& builder);
	};

}
