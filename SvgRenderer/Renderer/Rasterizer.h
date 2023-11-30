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

	struct TileIncrement
	{
		int16_t tileX;
		int16_t tileY;
		int8_t sign;
	};

	class Rasterizer
	{
	public:
		std::vector<Increment> increments;
		std::vector<TileIncrement> tileIncrements;
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
