#include "Flattening.h"

#include <glm/gtx/compatibility.hpp>

namespace SvgRenderer::Flattening {

	int32_t CalculateNumberOfSegments(const PathCmd& cmd, const glm::vec2& last, float tolerance)
	{
		switch (cmd.type)
		{
		case PathCmdType::MoveTo:
			return 1;
		case PathCmdType::LineTo:
			return 1;
		case PathCmdType::QuadTo:
		{
			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * cmd.as.quadTo.p1 + cmd.as.quadTo.p2)));
			return 1.0f / dt;
		}
		case PathCmdType::CubicTo:
		{
			const glm::vec2 a = -1.0f * last + 3.0f * cmd.as.cubicTo.p1 - 3.0f * cmd.as.cubicTo.p2 + cmd.as.cubicTo.p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * cmd.as.cubicTo.p1 + cmd.as.cubicTo.p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			return 1.0f / dt;
		}
		case PathCmdType::ConicTo:
			// TODO: Implement
			assert(false && "Not yet implemented");
			break;
		case PathCmdType::Close:
			assert(false && "Not yet implemented");
			break;
		default:
			assert(false && "Unknown path type");
			break;
		}
	}

	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance)
	{
		// TODO: Make this contained struct maybe a bit different, because it may only contain MoveTo and LineTo
		std::vector<PathCmd> simplePaths;

		switch (cmd.type)
		{
		case PathCmdType::MoveTo:
			simplePaths.emplace_back(cmd.as.moveTo);
			break;
		case PathCmdType::LineTo:
			simplePaths.emplace_back(cmd.as.lineTo);
			break;
		case PathCmdType::QuadTo:
		{
			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * cmd.as.quadTo.p1 + cmd.as.quadTo.p2)));
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, cmd.as.quadTo.p1, t);
				const glm::vec2 p12 = glm::lerp(cmd.as.quadTo.p1, cmd.as.quadTo.p2, t);
				const glm::vec2 p1 = glm::lerp(p01, p12, t);
				simplePaths.emplace_back(LineToCmd{ .p1 = p1 });
			}

			break;
		}
		case PathCmdType::CubicTo:
		{
			const glm::vec2 a = -1.0f * last + 3.0f * cmd.as.cubicTo.p1 - 3.0f * cmd.as.cubicTo.p2 + cmd.as.cubicTo.p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * cmd.as.cubicTo.p1 + cmd.as.cubicTo.p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, cmd.as.cubicTo.p1, t);
				const glm::vec2 p12 = glm::lerp(cmd.as.cubicTo.p1, cmd.as.cubicTo.p2, t);
				const glm::vec2 p23 = glm::lerp(cmd.as.cubicTo.p2, cmd.as.cubicTo.p3, t);
				const glm::vec2 p012 = glm::lerp(p01, p12, t);
				const glm::vec2 p123 = glm::lerp(p12, p23, t);
				const glm::vec2 p1 = glm::lerp(p012, p123, t);
				simplePaths.emplace_back(LineToCmd{ .p1 = p1 });
			}

			break;
		}
		case PathCmdType::ConicTo:
			// TODO: Implement
			assert(false && "Not yet implemented");
			break;
		case PathCmdType::Close:
			assert(false && "Not yet implemented");
			break;
		default:
			assert(false && "Unknown path type");
			break;
		}

		return simplePaths;
	}

}
