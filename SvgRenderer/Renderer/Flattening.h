#pragma once

#include <glm/glm.hpp>

#include <vector>

namespace SvgRenderer {

	struct PathCmd;
	struct PathRenderCmd;

}

namespace SvgRenderer::Flattening {

	std::vector<PathCmd> Flatten(const PathRenderCmd& cmd, glm::vec2 last, float tolerance);
	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance);

};
