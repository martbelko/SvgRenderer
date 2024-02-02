#pragma once

#include "Renderer/Path.h"

#include <glm/glm.hpp>

#include <vector>

namespace SvgRenderer::Flattening {

	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance);

};
