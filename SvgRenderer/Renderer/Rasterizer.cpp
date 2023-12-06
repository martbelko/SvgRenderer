#include "Rasterizer.h"

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

	void Rasterizer::MoveTo(const glm::vec2& point)
	{
		if (last != first)
		{
			LineTo(first);
		}

		first = point;
		last = point;
		prevTileY = glm::floor(point.y) / TILE_SIZE;
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
				rowt1 = std::min((dtdy * (nextY - last.y)), 1.0f);
			}

			float colt1;
			if (last.x == point.x)
			{
				colt1 = std::numeric_limits<float>::max();
			}
			else
			{
				float nextX = point.x > last.x ? x + 1 : x;
				colt1 = std::min((dtdx * (nextX - last.x)), 1.0f);
			}

			float xStep = std::abs(dtdx);
			float yStep = std::abs(dtdy);

			while (true)
			{
				float t0 = std::max(rowt0, colt0);
				float t1 = std::min(rowt1, colt1);
				glm::vec2 p0 = (1.0f - t0) * last + t0 * point;
				glm::vec2 p1 = (1.0f - t1) * last + t1 * point;
				float height = p1.y - p0.y;
				float right = x + 1;
				float area = 0.5f * height * ((right - p0.x) + (right - p1.x));

				increments.push_back(Increment{
					.x = x,
					.y = y,
					.area = area,
					.height = height
					});

				// Advance to the next scanline
				if (rowt1 < colt1)
				{
					rowt0 = rowt1;
					rowt1 = std::min((rowt1 + yStep), 1.0f);
					y += yDir;
				}
				else
				{
					colt0 = colt1;
					colt1 = std::min((colt1 + xStep), 1.0f);
					x += xDir;
				}

				// Reset coordinates if a scanline is completed
				if (rowt0 == 1.0f || colt0 == 1.0f)
				{
					x = point.x;
					y = point.y;
				}

				// Handle tile boundaries
				int32_t tileY = y / TILE_SIZE;
				if (tileY != prevTileY)
				{
					int32_t v1 = x / TILE_SIZE; // Find out which tile index on x-axis are we on
					int8_t v2 = tileY - prevTileY; // Are we moving from top to bottom, or bottom to top? (1 = from lower tile to higher tile, -1 = opposite)
					uint32_t currentTileY = v2 == 1 ? prevTileY : tileY;

					size_t currentIndex = std::max(std::min(GetTileIndex(v1, currentTileY), tiles.size() - 1), static_cast<size_t>(0));

					for (size_t i = 0; i < currentIndex; ++i)
						tiles[i].winding += v2;

					prevTileY = tileY;
				}

				// Break the loop if scanline or column completion condition is met
				if (rowt0 == 1.0f || colt0 == 1.0f)
					break;
			}
		}

		last = point;
	}

	void Rasterizer::Command(const PathCmd& command)
	{
		command.Flatten(last, TOLERANCE, [this](const PathCmd& cmd)
		{
			switch (cmd.type)
			{
			case PathCmdType::MoveTo:
				this->MoveTo(cmd.as.moveTo.point);
				break;
			case PathCmdType::LineTo:
				this->LineTo(cmd.as.lineTo.p1);
				break;
			default:
				assert(false && "Only moves and lines");
				break;
			}
		});
	}

	void Rasterizer::Fill(const std::vector<PathCmd>& path)
	{
		for (const PathCmd& cmd : path)
		{
			Command(cmd);
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

		// Increments may be in any order

		std::sort(increments.begin(), increments.end(), [](const Increment& inc1, const Increment& inc2)
		{
			const int32_t tile1X = inc1.x / TILE_SIZE;
			const int32_t tile1Y = inc1.y / TILE_SIZE;
			const int32_t tile2X = inc2.x / TILE_SIZE;
			const int32_t tile2Y = inc2.y / TILE_SIZE;
			return std::tie(tile1Y, tile1X) < std::tie(tile2Y, tile2X);
		});

		Bin* currentBin = &GetBin(0, 0);
		if (!increments.empty())
			currentBin = &GetBin(increments[0].x / TILE_SIZE, increments[0].y / TILE_SIZE);

		for (size_t i = 0; i < increments.size(); ++i)
		{
			const Increment& increment = increments[i];
			const int32_t tileX = increment.x / TILE_SIZE;
			const int32_t tileY = increment.y / TILE_SIZE;

			if (tileX != currentBin->tileX || tileY != currentBin->tileY)
			{
				// TODO: Check if tile indices are in valid range
				currentBin = &GetBin(tileX, tileY);
				currentBin->start = i;
				currentBin->end = i;
			}

			++(currentBin->end);
		}

		auto filterBins = [&]()
			{
				std::vector<Bin> result;
				for (const Bin& bin : bins)
				{
					if (bin.end != bin.start)
						result.push_back(bin);
				}

				return result;
			};

		bins = filterBins();

		for (size_t i = 0; i < bins.size(); i++)
		{
			const Bin& bin = bins[i];
			if (bin.start == bin.end)
				continue;

			Bin* nextBin = nullptr;
			for (size_t j = i + 1; j < bins.size() && bins[j].tileY == bin.tileY; ++j)
			{
				if (bins[j].start != bins[j].end)
				{
					nextBin = &bins[j];
					break;
				}
			}

			if (nextBin != nullptr)
			{
				// If the winding is nonzero, span the whole tile
				if (GetTile(bin.tileX, bin.tileY).winding != 0)
				{
					int32_t width = nextBin->tileX - bin.tileX - 1;
					builder.Span((bin.tileX + 1) * TILE_SIZE, bin.tileY * TILE_SIZE, width * TILE_SIZE);
				}
			}
		}

		std::array<float, TILE_SIZE * TILE_SIZE> areas{};
		std::array<float, TILE_SIZE * TILE_SIZE> heights{};
		std::array<float, TILE_SIZE> coverage{};

		int ctr = 0;

		for (size_t i = 0; i < bins.size(); ++i)
		{
			++ctr;
			const Bin& bin = bins[i];
			if (bin.end == bin.start && ctr < 7000)
				continue;

			for (size_t i = bin.start; i < bin.end; ++i)
			{
				const Increment& increment = increments[i];
				// Loop over each increment inside the tile
				uint32_t x = increment.x % TILE_SIZE; // Pixel x-coord relative to the tile
				uint32_t y = increment.y % TILE_SIZE; // Pixel y-coord relative to the tile
				areas[y * TILE_SIZE + x] += increment.area; // Just simple increment for area
				heights[y * TILE_SIZE + x] += increment.height; // Just simple increment for height
			}

			std::array<uint8_t, TILE_SIZE * TILE_SIZE> tile;

			// For each y-coord in the tile
			for (uint32_t y = 0; y < TILE_SIZE; ++y)
			{
				float accum = coverage[y];

				// For each x-coord in the tile
				for (uint32_t x = 0; x < TILE_SIZE; ++x)
				{
					tile[y * TILE_SIZE + x] = glm::min(glm::abs(accum + areas[y * TILE_SIZE + x]) * 256.0f, 255.0f);
					accum += heights[y * TILE_SIZE + x];
				}

				coverage[y] = accum;
			}

			builder.Tile(bin.tileX * TILE_SIZE, bin.tileY * TILE_SIZE, tile);

			std::fill(areas.begin(), areas.end(), 0.0f);
			std::fill(heights.begin(), heights.end(), 0.0f);

			Bin* nextBin = nullptr; // Next active bin in the same y-coord, same as the previous one, could be optimized and only done once
			for (size_t j = i + 1; j < bins.size() && bins[j].tileY == bin.tileY; ++j)
			{
				if (bins[j].start != bins[j].end)
				{
					nextBin = &bins[j];
					break;
				}
			}

			if (nextBin == nullptr)
			{
				std::fill(coverage.begin(), coverage.end(), 0.0f);
			}
		}
	}

}
