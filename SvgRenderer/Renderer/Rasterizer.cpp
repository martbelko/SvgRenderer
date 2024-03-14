#include "Rasterizer.h"

#include "Renderer/Flattening.h"

#include <cassert>
#include <numeric>
#include <execution>

namespace SvgRenderer {

	static int8_t sign(float value)
	{
		if (value > 0)
			return 1;
		else if (value < 0)
			return -1;
		return 0;
	}

	Rasterizer::Rasterizer(const BoundingBox& bbox, uint32_t pi)
	{
		pathIndex = pi;

		const int32_t minBboxCoordX = glm::floor(bbox.min.x);
		const int32_t minBboxCoordY = glm::floor(bbox.min.y);
		const int32_t maxBboxCoordX = glm::ceil(bbox.max.x);
		const int32_t maxBboxCoordY = glm::ceil(bbox.max.y);

		const int32_t minTileCoordX = glm::floor(static_cast<float>(minBboxCoordX) / TILE_SIZE);
		const int32_t minTileCoordY = glm::floor(static_cast<float>(minBboxCoordY) / TILE_SIZE);
		const int32_t maxTileCoordX = glm::ceil(static_cast<float>(maxBboxCoordX) / TILE_SIZE);
		const int32_t maxTileCoordY = glm::ceil(static_cast<float>(maxBboxCoordY) / TILE_SIZE);

		m_TileStartX = minTileCoordX;
		m_TileStartY = minTileCoordY;
		m_TileCountX = maxTileCoordX - minTileCoordX + 1;
		m_TileCountY = maxTileCoordY - minTileCoordY + 1;
	}

	void Rasterizer::MoveTo(const glm::vec2& last, const glm::vec2& point)
	{
	}

	void Rasterizer::LineTo(const glm::vec2& last, const glm::vec2& point)
	{
		if (point != last)
		{
			uint32_t prevTileY = GetTileCoordY(glm::floor(last.y));

			int32_t xDir = sign(point.x - last.x);
			int32_t yDir = sign(point.y - last.y);
			float dtdx = 1.0f / (point.x - last.x);
			float dtdy = 1.0f / (point.y - last.y);
			int32_t x = glm::floor(last.x); // Convert to int
			int32_t y = glm::floor(last.y);  // Convert to int
			float rowt0 = 0.0f;
			float colt0 = 0.0f;

			float rowt1;
			if (last.y == point.y)
			{
				rowt1 = std::numeric_limits<float>::max();
			}
			else
			{
				float nextY = point.y > last.y ? y + 1 : y;
				rowt1 = glm::min((dtdy * (nextY - last.y)), 1.0f);
			}

			float colt1;
			if (last.x == point.x)
			{
				colt1 = std::numeric_limits<float>::max();
			}
			else
			{
				float nextX = point.x > last.x ? x + 1 : x;
				colt1 = glm::min((dtdx * (nextX - last.x)), 1.0f);
			}

			float xStep = glm::abs(dtdx);
			float yStep = glm::abs(dtdy);

			while (true)
			{
				float t0 = glm::max(rowt0, colt0);
				float t1 = glm::min(rowt1, colt1);
				glm::vec2 p0 = (1.0f - t0) * last + t0 * point;
				glm::vec2 p1 = (1.0f - t1) * last + t1 * point;
				float height = p1.y - p0.y;
				float right = x + 1;
				float area = 0.5f * height * ((right - p0.x) + (right - p1.x));

				int32_t relativeX = glm::abs(x % TILE_SIZE);
				int32_t relativeY = glm::abs(y % TILE_SIZE);
				if (x < 0)
				{
					relativeX = TILE_SIZE - relativeX - 1;
				}
				if (y < 0)
				{
					relativeY = TILE_SIZE - relativeY - 1;
				}

				{
					std::lock_guard lock(mut1);
					GetTileFromWindowPos(x, y).increments[relativeY * TILE_SIZE + relativeX].area += int32_t(area * 100.0f);
					GetTileFromWindowPos(x, y).increments[relativeY * TILE_SIZE + relativeX].height += int32_t(height * 100.0f);
					GetTileFromWindowPos(x, y).hasIncrements = true;
				}

				// Advance to the next scanline
				if (rowt1 < colt1)
				{
					rowt0 = rowt1;
					rowt1 = glm::min((rowt1 + yStep), 1.0f);
					y += yDir;
				}
				else
				{
					colt0 = colt1;
					colt1 = glm::min((colt1 + xStep), 1.0f);
					x += xDir;
				}

				// Reset coordinates if a scanline is completed
				if (rowt0 == 1.0f || colt0 == 1.0f)
				{
					x = glm::floor(point.x);
					y = glm::floor(point.y);
				}

				// Handle tile boundaries
				int32_t tileY = GetTileCoordY(y);
				if (tileY != prevTileY)
				{
					int32_t v1 = GetTileCoordX(x); // Find out which tile index on x-axis are we on
					int8_t v2 = tileY - prevTileY; // Are we moving from top to bottom, or bottom to top? (1 = from lower tile to higher tile, -1 = opposite)
					uint32_t currentTileY = v2 == 1 ? prevTileY : tileY;

					const PathRender& path = Globals::AllPaths.paths[pathIndex];
					uint32_t tileCount = path.endTileIndex - path.startTileIndex + 1;
					uint32_t currentIndex = glm::max(glm::min(GetTileIndexFromRelativePos(v1, currentTileY), static_cast<uint32_t>(tileCount - 1)), 0u);

					{
						std::lock_guard lock(mut2);
						for (uint32_t i = 0; i < currentIndex; i++)
						{
							Globals::Tiles.tiles[i + path.startTileIndex].winding += v2;
						}
					}

					prevTileY = tileY;
				}

				// Break the loop if we are on the end
				if (rowt0 == 1.0f || colt0 == 1.0f)
				{
					break;
				}
			}
		}
	}

	void Rasterizer::CommandFromArray(const PathRenderCmd& cmd, const glm::vec2& lastPoint)
	{
		std::vector<uint32_t> indices;
		indices.resize(cmd.endIndexSimpleCommands - cmd.startIndexSimpleCommands);
		std::iota(indices.begin(), indices.end(), cmd.startIndexSimpleCommands);

		auto GetSimpleCmdPrevPoint = [lastPoint, &cmd](uint32_t simpleCmdIndex) -> glm::vec2
		{
			return simpleCmdIndex == cmd.startIndexSimpleCommands ? lastPoint : Globals::AllPaths.simpleCommands[simpleCmdIndex - 1].point;
		};

		std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this, GetSimpleCmdPrevPoint](uint32_t i)
		{
			const SimpleCommand& simpleCmd = Globals::AllPaths.simpleCommands[i];
			glm::vec2 last = GetSimpleCmdPrevPoint(i);

			switch (simpleCmd.type)
			{
			case MOVE_TO:
				this->MoveTo(last, simpleCmd.point);
				break;
			case LINE_TO:
				this->LineTo(last, simpleCmd.point);
				break;
			default:
				assert(false && "Only moves and lines");
				break;
			}
		});
	}

	static glm::vec2 GetPreviousPoint(uint32_t pathIndex, uint32_t cmdIndex)
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		if (cmdIndex == path.startCmdIndex)
		{
			return glm::vec2(0, 0);
		}

		const PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex - 1];
		uint32_t pathType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		case LINE_TO:
			return cmd.transformedPoints[0];
		case QUAD_TO:
			return cmd.transformedPoints[1];
		case CUBIC_TO:
			return cmd.transformedPoints[2];
		}

		SR_ASSERT(false, "Invalid path type");
		return glm::vec2(0, 0);
	}

	void Rasterizer::FillFromArray(uint32_t pathIndex)
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];

		std::vector<uint32_t> indices;
		indices.resize(path.endCmdIndex - path.startCmdIndex + 1);
		std::iota(indices.begin(), indices.end(), 0);

		std::for_each(executionPolicy, indices.cbegin(), indices.cend(), [this, pathIndex, &path](uint32_t cmdIndex)
		{
			const PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex + path.startCmdIndex];
			glm::vec2 last = GetPreviousPoint(pathIndex, cmdIndex + path.startCmdIndex);
			CommandFromArray(cmd, last);
		});
	}

	std::pair<uint32_t, uint32_t> Rasterizer::CalculateNumberOfQuads()
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		uint32_t tileCount = path.endTileIndex - path.startTileIndex + 1;

		// Coarse
		uint32_t coarseQuadCount = 0;
		for (uint32_t i = 0; i < tileCount; i++)
		{
			Tile& tile = Globals::Tiles.tiles[i + path.startTileIndex];
			if (!tile.hasIncrements)
			{
				continue;
			}

			Tile* nextTile = nullptr;
			const int32_t tileY = GetTileYFromAbsoluteIndex(i);
			int32_t nextTileX;

			for (uint32_t j = i + 1; j < tileCount && GetTileYFromAbsoluteIndex(j) == tileY; j++)
			{
				if (Globals::Tiles.tiles[j + path.startTileIndex].hasIncrements)
				{
					nextTile = &Globals::Tiles.tiles[j + path.startTileIndex];
					nextTileX = GetTileXFromAbsoluteIndex(j);
					tile.nextTileIndex = j + path.startTileIndex;
					break;
				}
			}

			if (nextTile != nullptr)
			{
				const int32_t tileX = GetTileXFromAbsoluteIndex(i);
				int32_t nextTileX = GetTileXFromAbsoluteIndex(tile.nextTileIndex - path.startTileIndex);
				int32_t width = nextTileX - tileX - 1;
				// If the winding is nonzero, span the whole tile
				if (tileX + width + 1 >= 0 && tileY >= 0 && tileY <= glm::ceil(float(SCREEN_HEIGHT) / TILE_SIZE)
				    && GetTileFromRelativePos(tileX - m_TileStartX, tileY - m_TileStartY).winding != 0)
				{
					coarseQuadCount++;
				}
			}
		}

		// Fine
		uint32_t fineQuadCount = 0;
		for (uint32_t i = 0; i < tileCount; i++)
		{
			const Tile& tile = Globals::Tiles.tiles[i + path.startTileIndex];
			if (!tile.hasIncrements)
			{
				continue;
			}

			int32_t tileX = GetTileXFromAbsoluteIndex(i);
			int32_t tileY = GetTileYFromAbsoluteIndex(i);
			if (tileX >= 0 && tileY >= 0 && tileX <= glm::ceil(float(SCREEN_WIDTH) / TILE_SIZE) && tileY <= glm::ceil(float(SCREEN_HEIGHT) / TILE_SIZE))
			{
				fineQuadCount++;
			}
		}

		return std::make_pair(coarseQuadCount, fineQuadCount);
	}

	void Rasterizer::Coarse(TileBuilder& builder)
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		uint32_t tileCount = path.endTileIndex - path.startTileIndex + 1;
		uint32_t quadIndex = path.startSpanQuadIndex;

		for (uint32_t i = 0; i < tileCount; i++)
		{
			const Tile& tile = Globals::Tiles.tiles[i + path.startTileIndex];
			if (!tile.hasIncrements)
			{
				continue;
			}

			Tile* nextTile = tile.nextTileIndex == std::numeric_limits<uint32_t>::max() ? nullptr : &Globals::Tiles.tiles[tile.nextTileIndex];
			if (nextTile != nullptr)
			{
				const int32_t tileX = GetTileXFromAbsoluteIndex(i);
				const int32_t tileY = GetTileYFromAbsoluteIndex(i);
				int32_t nextTileX = GetTileXFromAbsoluteIndex(tile.nextTileIndex - path.startTileIndex);
				int32_t width = nextTileX - tileX - 1;
				// If the winding is nonzero, span the whole tile
				if (tileX + width + 1 >= 0 && tileY >= 0 && tileY <= glm::ceil(float(SCREEN_HEIGHT) / TILE_SIZE)
				    && GetTileFromRelativePos(tileX - m_TileStartX, tileY - m_TileStartY).winding != 0)
				{
					builder.Span((tileX + 1) * TILE_SIZE, tileY * TILE_SIZE, width * TILE_SIZE, quadIndex++, Globals::AllPaths.paths[pathIndex].color);
				}
			}
		}
	}

	void Rasterizer::Finish(TileBuilder& builder)
	{
		std::array<float, TILE_SIZE * TILE_SIZE> areas{};
		std::array<float, TILE_SIZE * TILE_SIZE> heights{};
		std::array<float, TILE_SIZE> coverage{};

		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		uint32_t tileCount = path.endTileIndex - path.startTileIndex + 1;
		uint32_t quadIndex = path.startTileQuadIndex;
		uint32_t tileIndex = path.startVisibleTileIndex;

		for (uint32_t i = 0; i < tileCount; i++)
		{
			const Tile& tile = Globals::Tiles.tiles[i + path.startTileIndex];
			if (!tile.hasIncrements)
			{
				continue;
			}

			for (int32_t y = 0; y < TILE_SIZE; y++)
			{
				for (int32_t x = 0; x < TILE_SIZE; x++)
				{
					const Increment& increment = tile.increments[y * TILE_SIZE + x];
					// Loop over each increment inside the tile
					areas[y * TILE_SIZE + x] = increment.area / 100.0f; // Just simple increment for area
					heights[y * TILE_SIZE + x] = increment.height / 100.0f; // Just simple increment for height
				}
			}

			std::array<uint8_t, TILE_SIZE * TILE_SIZE> tileData;

			// For each y-coord in the tile
			for (uint32_t y = 0; y < TILE_SIZE; y++)
			{
				float accum = coverage[y];

				// For each x-coord in the tile
				for (uint32_t x = 0; x < TILE_SIZE; x++)
				{
					tileData[y * TILE_SIZE + x] = glm::min(glm::abs(accum + areas[y * TILE_SIZE + x]) * 256.0f, 255.0f);
					accum += heights[y * TILE_SIZE + x];
				}

				coverage[y] = accum;
			}

			int32_t tileX = GetTileXFromAbsoluteIndex(i);
			int32_t tileY = GetTileYFromAbsoluteIndex(i);
			if (tileX >= 0 && tileY >= 0 && tileX <= glm::ceil(float(SCREEN_WIDTH) / TILE_SIZE) && tileY <= glm::ceil(float(SCREEN_HEIGHT) / TILE_SIZE))
			{
				builder.Tile(tileX * TILE_SIZE, tileY * TILE_SIZE, tileData, tileIndex++, quadIndex++, Globals::AllPaths.paths[pathIndex].color);
			}

			std::fill(areas.begin(), areas.end(), 0.0f);
			std::fill(heights.begin(), heights.end(), 0.0f);

			// Next active tile in the same y-coord, same as the previous one, could be optimized and only done once
			Tile* nextTile = tile.nextTileIndex == std::numeric_limits<uint32_t>::max() ? nullptr : &Globals::Tiles.tiles[tile.nextTileIndex];
			if (nextTile == nullptr)
			{
				std::fill(coverage.begin(), coverage.end(), 0.0f);
			}
		}
	}

}
