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
		//std::cout << "MoveTo: " << point.x << ";" << point.y << '\n';
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
		//std::cout << "LineTo: " << point.x << ";" << point.y << '\n';

		if (point != last)
		{
			int16_t xDir = sign(point.x - last.x);
			int16_t yDir = sign(point.y - last.y);
			float dtdx = 1.0f / (point.x - last.x);
			float dtdy = 1.0f / (point.y - last.y);
			int16_t x = last.x; // Convert to int
			int16_t y = last.y; // Convert to int
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
				int16_t tileY = y / TILE_SIZE;
				if (tileY != prevTileY)
				{
					int16_t v1 = x / TILE_SIZE; // Find out which tile index on x-axis are we on
					int8_t v2 = tileY - prevTileY; // Are we moving from top to bottom, or bottom to top? (1 = from lower tile to higher tile, 0 = opposite)
					tileIncrements.push_back(TileIncrement{
						.tileX = v1, // Set the x-axis tile index
						.tileY = v2 == 1 ? prevTileY : tileY, // This is the same as std::min(prevTileY, tileY)
						.sign = v2 // Set the sign correctly, so we know which tile was previous one
					});
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

	void Rasterizer::Fill(const std::vector<PathCmd>& path, const glm::mat3& transform)
	{
		for (const PathCmd& cmd : path)
		{
			Command(cmd.Transform(transform));
		}
	}

	void Rasterizer::Stroke(const std::vector<PathCmd>& path, float width, const glm::mat3& transform)
	{
		Fill(PathStroke(PathFlatten(path, TOLERANCE), width), transform);
	}

	void Rasterizer::Finish(TileBuilder& builder)
	{
		if (last != first)
		{
			LineTo(first);
		}

		// Increments may be in any order

		struct Bin
		{
			int16_t tileX;
			int16_t tileY;
			size_t start;
			size_t end;

			bool operator<(const Bin& other) const
			{
				return std::tie(tileY, tileX) < std::tie(other.tileY, other.tileX);
			}
		};

		std::vector<Bin> bins;
		Bin bin = Bin{
			.tileX = 0,
			.tileY = 0,
			.start = 0,
			.end = 0
		};

		if (!increments.empty())
		{
			bin.tileX = (int16_t)first.x / TILE_SIZE;
			bin.tileY = (int16_t)first.y / TILE_SIZE;
		}

		for (size_t i = 0; i < increments.size(); ++i)
		{
			const Increment& increment = increments[i];
			const int16_t tileX = increment.x / TILE_SIZE;
			const int16_t tileY = increment.y / TILE_SIZE;
			if (tileX != bin.tileX || tileY != bin.tileY)
			{
				bins.push_back(std::move(bin));
				bin = Bin{
					.tileX = tileX,
					.tileY = tileY,
					.start = i,
					.end = i
				};
			}

			++bin.end;
		}

		bins.push_back(std::move(bin));

		std::sort(bins.begin(), bins.end(), [](const Bin& bin1, const Bin& bin2)
		{
			return std::tie(bin1.tileY, bin1.tileX) < std::tie(bin2.tileY, bin2.tileX);
		});

		std::sort(tileIncrements.begin(), tileIncrements.end(), [](const TileIncrement& inc1, const TileIncrement& inc2)
		{
			return std::tie(inc1.tileY, inc1.tileX) < std::tie(inc2.tileY, inc2.tileX);
		});

		size_t tileIncrementsIndex = 0;
		int32_t winding = 0;

		for (size_t i = 0; i < bins.size(); ++i)
		{
			const Bin& bin = bins[i];
			if (i + 1 == bins.size() || bins[i + 1].tileX != bin.tileX || bins[i + 1].tileY != bin.tileY)
			{
				if (i + 1 < bins.size() && bins[i + 1].tileY == bin.tileY && bins[i + 1].tileX > bin.tileX + 1)
				{
					while (tileIncrementsIndex < tileIncrements.size())
					{
						const TileIncrement& tileIncrement = tileIncrements[tileIncrementsIndex];
						if (std::tie(tileIncrement.tileY, tileIncrement.tileX) > std::tie(bin.tileY, bin.tileX))
						{
							break;
						}

						winding += tileIncrement.sign; // Accumulate winding according to the sign
						++tileIncrementsIndex; // Increase the index for tile_increments buffer
					}

					// If the winding is nonzero, span the whole tile
					if (winding != 0)
					{
						int16_t width = bins[i + 1].tileX - bin.tileX - 1;
						builder.Span((bin.tileX + 1) * TILE_SIZE, bin.tileY * TILE_SIZE, width * TILE_SIZE);
					}
				}
			}
		}

		std::array<float, TILE_SIZE * TILE_SIZE> areas{};
		std::array<float, TILE_SIZE * TILE_SIZE> heights{};
		std::array<float, TILE_SIZE> prev{};
		std::array<float, TILE_SIZE> next{};

		for (size_t i = 0; i < bins.size(); ++i)
		{
			const Bin& bin = bins[i];

			for (size_t i = bin.start; i < bin.end; ++i)
			{
				const Increment& increment = increments[i];
				// Loop over each increment inside the tile
				uint32_t x = increment.x % TILE_SIZE; // Pixel x-coord relative to the tile
				uint32_t y = increment.y % TILE_SIZE; // Pixel y-coord relative to the tile
				areas[y * TILE_SIZE + x] += increment.area; // Just simple increment for area
				heights[y * TILE_SIZE + x] += increment.height; // Just simple increment for height
			}

			// If we are on the end of bins, or the next tile has different position as the current tile
			// Basically, we are accumulating areas and heights until we are moving to the bin on next position
			if (i + 1 == bins.size() || bins[i + 1].tileX != bin.tileX || bins[i + 1].tileY != bin.tileY)
			{
				std::array<uint8_t, TILE_SIZE * TILE_SIZE> tile{};

				// For each y-coord in the tile
				for (uint32_t y = 0; y < TILE_SIZE; ++y)
				{
					float accum = prev[y];

					// For each x-coord in the tile
					for (uint32_t x = 0; x < TILE_SIZE; ++x)
					{
						tile[y * TILE_SIZE + x] = glm::min(glm::abs(accum + areas[y * TILE_SIZE + x]) * 256.0f, 255.0f);
						accum += heights[y * TILE_SIZE + x];
					}

					next[y] = accum; // This accum is what he calls cover table
				}

				builder.Tile(bin.tileX * TILE_SIZE, bin.tileY * TILE_SIZE, tile);

				std::fill(areas.begin(), areas.end(), 0.0f);
				std::fill(heights.begin(), heights.end(), 0.0f);

				// If this is not the last tile, and the next one is on the same y-coord
				if (i + 1 < bins.size() && bins[i + 1].tileY == bin.tileY)
				{
					prev = next;
				}
				else
				{
					std::fill(prev.begin(), prev.end(), 0.0f);
				}

				std::fill(next.begin(), next.end(), 0.0f);
			}
		}
	}

}
