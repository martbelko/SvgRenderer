#include "Flattening.h"

#include "Renderer/Defs.h"
#include "Renderer/Path.h"

#include <glm/gtx/compatibility.hpp>

namespace SvgRenderer::Flattening {

	int32_t CalculateNumberOfSimpleCommands(const PathCmd& cmd, const glm::vec2& last, float tolerance)
	{
		switch (cmd.type)
		{
		case PathCmdType::MoveTo:
			return 1;
		case PathCmdType::LineTo:
			return 1;
		case PathCmdType::QuadTo:
		{
			const float dt = glm::sqrt((4.0f * tolerance) / glm::length(last - 2.0f * cmd.as.quadTo.p1 + cmd.as.quadTo.p2));
			return glm::ceil(1.0f / dt);
		}
		case PathCmdType::CubicTo:
		{
			const glm::vec2 a = -1.0f * last + 3.0f * cmd.as.cubicTo.p1 - 3.0f * cmd.as.cubicTo.p2 + cmd.as.cubicTo.p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * cmd.as.cubicTo.p1 + cmd.as.cubicTo.p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			return glm::ceil(1.0f / dt);
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

		return 0;
	}

	int32_t CalculateNumberOfSimpleCommands(const PathRenderCmd& cmd, const glm::vec2& last, float tolerance)
	{
		uint32_t pathType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
			return 1;
		case LINE_TO:
			return 1;
		case QUAD_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];

			bool isVisible = IsInsideViewSpace(last) || IsInsideViewSpace(p1) || IsInsideViewSpace(p2);
			if (!isVisible)
			{
				return 1;
			}

			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * p1 + p2)));
			return glm::ceil(1.0f / dt);
		}
		case CUBIC_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];
			const glm::vec2& p3 = cmd.transformedPoints[2];

			bool isVisible = IsInsideViewSpace(last) || IsInsideViewSpace(p1) || IsInsideViewSpace(p2) || IsInsideViewSpace(p3);
			if (!isVisible)
			{
				return 1;
			}

			const glm::vec2 a = -1.0f * last + 3.0f * p1 - 3.0f * p2 + p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * p1 + p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			if (conc == 0.0f)
			{
				return 1;
			}

			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			return glm::ceil(1.0f / dt);
		}
		}

		return 0;
	}

	bool IsInsideViewSpace(const glm::vec2& v)
	{
		// TODO: This needs to be fixed, and actually checked for bunch of points, if the polygon
		// created out of these points lie completely outside of the view space (screen)
		return true;
		constexpr float padding = 1.0f;
		return !(v.x > 1900 + padding || v.x < 0 - padding || v.y > 1000 + padding || v.y < 0 - padding);
	}

	BoundingBox FlattenIntoArray(const PathRenderCmd& cmd, glm::vec2 last, float tolerance)
	{
		uint32_t index = cmd.startIndexSimpleCommands;

		BoundingBox bbox;
		uint32_t pathType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		{
			Globals::AllPaths.simpleCommands[index] = SimpleCommand{ .type = MOVE_TO, .point = cmd.transformedPoints[0] };
			bbox.AddPoint(cmd.transformedPoints[0]);
			break;
		}
		case LINE_TO:
		{
			const glm::vec2& p = cmd.transformedPoints[0];
			Globals::AllPaths.simpleCommands[index] = SimpleCommand{ .type = LINE_TO, .point = p };
			bbox.AddPoint(p);
			break;
		}
		case QUAD_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];

			bool isVisible = IsInsideViewSpace(last) || IsInsideViewSpace(p1) || IsInsideViewSpace(p2);
			if (!isVisible)
			{
				Globals::AllPaths.simpleCommands[index++] = SimpleCommand{ .type = LINE_TO, .point = p2 };
				break;
			}

			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * p1 + p2)));
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 p1 = glm::lerp(p01, p12, t);

				Globals::AllPaths.simpleCommands[index++] = SimpleCommand{ .type = LINE_TO, .point = p1 };
				bbox.AddPoint(p1);
			}

			break;
		}
		case CUBIC_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];
			const glm::vec2& p3 = cmd.transformedPoints[2];

			bool isVisible = IsInsideViewSpace(last) || IsInsideViewSpace(p1) || IsInsideViewSpace(p2) || IsInsideViewSpace(p3);
			if (!isVisible)
			{
				Globals::AllPaths.simpleCommands[index++] = SimpleCommand{ .type = LINE_TO, .point = p3 };
				break;
			}

			const glm::vec2 a = -1.0f * last + 3.0f * p1 - 3.0f * p2 + p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * p1 + p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			float t = 0.0f;

			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 p23 = glm::lerp(p2, p3, t);
				const glm::vec2 p012 = glm::lerp(p01, p12, t);
				const glm::vec2 p123 = glm::lerp(p12, p23, t);
				const glm::vec2 p1 = glm::lerp(p012, p123, t);

				Globals::AllPaths.simpleCommands[index++] = SimpleCommand{ .type = LINE_TO, .point = p1 };
				bbox.AddPoint(p1);
			}

			break;
		}
		default:
			assert(false && "Unknown path type");
			break;
		}

		return bbox;
	}

	std::vector<PathCmd> Flatten(const PathRenderCmd& cmd, glm::vec2 last, float tolerance)
	{
		// TODO: Make this contained struct maybe a bit different, because it may only contain MoveTo and LineTo
		int32_t expectedSize = CalculateNumberOfSimpleCommands(cmd, last, tolerance); // Expected number of simple commands

		std::vector<PathCmd> simplePaths;
		simplePaths.reserve(expectedSize);

		uint32_t pathType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
			simplePaths.push_back(MoveToCmd{ .point = cmd.transformedPoints[0] });
			break;
		case LINE_TO:
			simplePaths.push_back(LineToCmd{ .p1 = cmd.transformedPoints[0] });
			break;
		case QUAD_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];

			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * p1 + p2)));
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 p1 = glm::lerp(p01, p12, t);
				simplePaths.push_back(LineToCmd{ .p1 = p1 });
			}

			break;
		}
		case CUBIC_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];
			const glm::vec2& p3 = cmd.transformedPoints[2];

			const glm::vec2 a = -1.0f * last + 3.0f * p1 - 3.0f * p2 + p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * p1 + p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 p23 = glm::lerp(p2, p3, t);
				const glm::vec2 p012 = glm::lerp(p01, p12, t);
				const glm::vec2 p123 = glm::lerp(p12, p23, t);
				const glm::vec2 p1 = glm::lerp(p012, p123, t);
				simplePaths.push_back(LineToCmd{ .p1 = p1 });
			}

			break;
		}
		default:
			//assert(false && "Unknown path type");
			break;
		}

		assert(simplePaths.size() == expectedSize);
		return simplePaths;
	}

	std::vector<PathCmd> Flatten(const PathCmd& cmd, glm::vec2 last, float tolerance)
	{
		// TODO: Make this contained struct maybe a bit different, because it may only contain MoveTo and LineTo
		int32_t expectedSize = CalculateNumberOfSimpleCommands(cmd, last, tolerance);

		std::vector<PathCmd> simplePaths;
		simplePaths.reserve(expectedSize);

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

		assert(simplePaths.size() == expectedSize);
		return simplePaths;
	}

}
