#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace SvgRenderer {

	struct PathCmd;
	struct PathRenderCmd;

}

namespace SvgRenderer::Flattening {

	int32_t CalculateNumberOfSimpleCommands(const PathRenderCmd& cmd, const glm::vec2& last, float tolerance);
	int32_t CalculateNumberOfSimpleCommands(const PathCmd& cmd, const glm::vec2& last, float tolerance);

	void FlattenIntoArray(const PathRenderCmd& cmd, glm::vec2 last, float tolerance);
	std::vector<PathCmd> Flatten(const PathRenderCmd& cmd, glm::vec2 last, float tolerance);
	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance);

};
