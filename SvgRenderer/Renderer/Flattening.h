#pragma once

#include "Utils/BoundingBox.h"

#include <glm/glm.hpp>

#include <vector>

namespace SvgRenderer {

	struct PathCmd;
	struct PathRenderCmd;
	struct SimpleCommand;

}

namespace SvgRenderer::Flattening {

	glm::vec2 ProjectPointOntoScreenBoundary(glm::vec2 point);

	int32_t CalculateNumberOfSimpleCommands(const PathRenderCmd& cmd, const glm::vec2& last, float tolerance);
	int32_t CalculateNumberOfSimpleCommands(const PathCmd& cmd, const glm::vec2& last, float tolerance);

	std::vector<SimpleCommand> Flatten(uint32_t cmdIndex, glm::vec2 last, float tolerance);
	uint32_t CalculateNumberOfSimpleCommands(uint32_t cmdIndex, glm::vec2 last, float tolerance);

	BoundingBox FlattenIntoArray(const PathRenderCmd& cmd, glm::vec2 last, float tolerance);
	std::vector<PathCmd> Flatten(const PathRenderCmd& cmd, glm::vec2 last, float tolerance);
	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance);

	bool IsPointInsideViewSpace(const glm::vec2& v);
	bool IsLineInsideViewSpace(glm::vec2 p0, glm::vec2 p1);
	bool IsBboxInsideViewSpace(const BoundingBox& bbox);

};
