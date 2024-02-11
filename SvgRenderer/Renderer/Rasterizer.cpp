#include "Rasterizer.h"

#include "Renderer/Defs.h"
#include "Renderer/Flattening.h"

#include <cassert>

namespace SvgRenderer {

	static int8_t sign(float value)
	{
		if (value > 0)
			return 1;
		else if (value < 0)
			return -1;
		return 0;
	}

	Rasterizer::Rasterizer(const BoundingBox& bbox)
	{
		const int32_t minBboxCoordX = glm::floor(bbox.min.x);
		const int32_t minBboxCoordY = glm::floor(bbox.min.y);
		const int32_t maxBboxCoordX = glm::ceil(bbox.max.x);
		const int32_t maxBboxCoordY = glm::ceil(bbox.max.y);

		const int32_t minTileCoordX = minBboxCoordX / TILE_SIZE;
		const int32_t minTileCoordY = minBboxCoordY / TILE_SIZE;
		const int32_t maxTileCoordX = glm::ceil(static_cast<float>(maxBboxCoordX) / TILE_SIZE);
		const int32_t maxTileCoordY = glm::ceil(static_cast<float>(maxBboxCoordY) / TILE_SIZE);

		m_TileStartX = minTileCoordX;
		m_TileStartY = minTileCoordY;
		m_TileCountX = maxTileCoordX - minTileCoordX + 1;
		m_TileCountY = maxTileCoordY - minTileCoordY + 1;

		const uint32_t tileCount = m_TileCountX * m_TileCountY;
		tiles.reserve(tileCount);

		for (int32_t y = minTileCoordY; y <= maxTileCoordY; y++)
		{
			for (int32_t x = minTileCoordX; x <= maxTileCoordX; x++)
			{
				tiles.push_back(Tile{
					.tileX = x,
					.tileY = y,
					.winding = 0,
					.hasIncrements = false,
					.increments = std::array<Increment, TILE_SIZE * TILE_SIZE>()
				});

				Tile& tile = tiles.back();
				for (int32_t relativeY = 0; relativeY < TILE_SIZE; relativeY++)
				{
					for (int32_t relativeX = 0; relativeX < TILE_SIZE; relativeX++)
					{
						tile.increments[relativeY * TILE_SIZE + relativeX].x = relativeX;
						tile.increments[relativeY * TILE_SIZE + relativeX].y = relativeY;
						tile.increments[relativeY * TILE_SIZE + relativeX].area = 0;
						tile.increments[relativeY * TILE_SIZE + relativeX].height = 0;
					}
				}
			}
		}
	}

	void Rasterizer::MoveTo(const glm::vec2& point)
	{
		if (last != first)
		{
			LineTo(first);
		}

		first = point;
		last = point;
		prevTileY = GetTileCoordY(glm::floor(point.y));
	}

	void Rasterizer::LineTo(const glm::vec2& point)
	{
		if (point != last)
		{
			int32_t xDir = sign(point.x - last.x);
			int32_t yDir = sign(point.y - last.y);
			float dtdx = 1.0f / (point.x - last.x);
			float dtdy = 1.0f / (point.y - last.y);
			int32_t x = last.x; // Convert to int
			int32_t y = last.y; // Convert to int
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

				int32_t relativeX = x % TILE_SIZE;
				int32_t relativeY = y % TILE_SIZE;

				GetTileFromWindowPos(x, y).increments[relativeY * TILE_SIZE + relativeX].area += area;
				GetTileFromWindowPos(x, y).increments[relativeY * TILE_SIZE + relativeX].height += height;
				GetTileFromWindowPos(x, y).hasIncrements = true;

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
					x = point.x;
					y = point.y;
				}

				// Handle tile boundaries
				int32_t tileY = GetTileCoordY(y);
				if (tileY != prevTileY)
				{
					int32_t v1 = GetTileCoordX(x); // Find out which tile index on x-axis are we on
					int8_t v2 = tileY - prevTileY; // Are we moving from top to bottom, or bottom to top? (1 = from lower tile to higher tile, -1 = opposite)
					uint32_t currentTileY = v2 == 1 ? prevTileY : tileY;

					// size_t currentIndex = glm::max(glm::min(GetTileIndexFromRelativePos(v1, currentTileY), static_cast<uint32_t>(tiles.size() - 1)), 0u);
					size_t currentIndex = GetTileIndexFromRelativePos(v1, currentTileY);
					for (size_t i = 0; i < currentIndex; i++)
					{
						tiles[i].winding += v2;
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

		last = point;
	}

	void Rasterizer::CommandFromArray(const PathRenderCmd& cmd, const glm::vec2& lastPoint)
	{
		for (uint32_t i = cmd.startIndexSimpleCommands; i <= cmd.endIndexSimpleCommands; i++)
		{
			const SimpleCommand& simpleCmd = Globals::AllPaths.simpleCommands[i];
			switch (simpleCmd.type)
			{
			case MOVE_TO:
				this->MoveTo(simpleCmd.point);
				break;
			case LINE_TO:
				this->LineTo(simpleCmd.point);
				break;
			default:
				assert(false && "Only moves and lines");
				break;
			}
		}
	}

	void Rasterizer::FillFromArray(uint32_t pathIndex)
	{
		const PathRender& path = Globals::AllPaths.paths[pathIndex];

		glm::vec2 last = glm::vec2(0, 0);
		for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
		{
			const PathRenderCmd& rndCmd = Globals::AllPaths.commands[i];
			CommandFromArray(rndCmd, last);

			uint32_t pathType = GET_CMD_TYPE(rndCmd.pathIndexCmdType);
			switch (pathType)
			{
			case MOVE_TO:
				last = rndCmd.transformedPoints[0];
				break;
			case LINE_TO:
				last = rndCmd.transformedPoints[0];
				break;
			case QUAD_TO:
				last = rndCmd.transformedPoints[1];
				break;
			case CUBIC_TO:
				last = rndCmd.transformedPoints[2];
				break;
			}
		}
	}

	void Rasterizer::Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform)
	{
		//Fill(PathStroke(PathFlatten(path, TOLERANCE), width), transform);
	}

	void Rasterizer::Finish(TileBuilder& builder)
	{
		if (last != first)
		{
			LineTo(first);
		}

		auto filterTiles = [&]()
		{
			std::vector<Tile> result;
			for (const Tile& tile : tiles)
			{
				if (tile.hasIncrements)
				{
					result.push_back(tile);
				}
			}

			return result;
		};
		auto filteredTiles = filterTiles();

		for (size_t i = 0; i < filteredTiles.size(); i++)
		{
			const Tile& tile = filteredTiles[i];
			if (!tile.hasIncrements)
			{
				continue;
			}

			Tile* nextTile = nullptr;
			for (size_t j = i + 1; j < filteredTiles.size() && filteredTiles[j].tileY == tile.tileY; j++)
			{
				if (filteredTiles[j].hasIncrements)
				{
					nextTile = &filteredTiles[j];
					break;
				}
			}

			if (nextTile != nullptr)
			{
				// If the winding is nonzero, span the whole tile
				if (GetTileFromRelativePos(tile.tileX - m_TileStartX, tile.tileY - m_TileStartY).winding != 0)
				{
					int32_t width = nextTile->tileX - tile.tileX - 1;
					builder.Span((tile.tileX + 1) * TILE_SIZE, tile.tileY * TILE_SIZE, width * TILE_SIZE);
				}
			}
		}

		std::array<float, TILE_SIZE * TILE_SIZE> areas{};
		std::array<float, TILE_SIZE * TILE_SIZE> heights{};
		std::array<float, TILE_SIZE> coverage{};

		for (size_t i = 0; i < filteredTiles.size(); ++i)
		{
			const Tile& tile = filteredTiles[i];
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
					areas[y * TILE_SIZE + x] = increment.area; // Just simple increment for area
					heights[y * TILE_SIZE + x] = increment.height; // Just simple increment for height
				}
			}

			std::array<uint8_t, TILE_SIZE * TILE_SIZE> tileData;

			// For each y-coord in the tile
			for (uint32_t y = 0; y < TILE_SIZE; ++y)
			{
				float accum = coverage[y];

				// For each x-coord in the tile
				for (uint32_t x = 0; x < TILE_SIZE; ++x)
				{
					tileData[y * TILE_SIZE + x] = glm::min(glm::abs(accum + areas[y * TILE_SIZE + x]) * 256.0f, 255.0f);
					accum += heights[y * TILE_SIZE + x];
				}

				coverage[y] = accum;
			}

			builder.Tile(tile.tileX * TILE_SIZE, tile.tileY * TILE_SIZE, tileData);

			std::fill(areas.begin(), areas.end(), 0.0f);
			std::fill(heights.begin(), heights.end(), 0.0f);

			Tile* nextTile = nullptr; // Next active bin in the same y-coord, same as the previous one, could be optimized and only done once
			for (size_t j = i + 1; j < filteredTiles.size() && filteredTiles[j].tileY == tile.tileY; ++j)
			{
				if (filteredTiles[j].hasIncrements)
				{
					nextTile = &filteredTiles[j];
					break;
				}
			}

			if (nextTile == nullptr)
			{
				std::fill(coverage.begin(), coverage.end(), 0.0f);
			}
		}
	}

}
