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

			bool isVisible = IsLineInsideViewSpace(last, p1) || IsLineInsideViewSpace(p1, p2) || IsLineInsideViewSpace(p2, last);
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

			bool isVisible = IsLineInsideViewSpace(last, p1) || IsLineInsideViewSpace(p1, p2) || IsLineInsideViewSpace(p2, p3) || IsLineInsideViewSpace(p3, last);
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

	bool IsPointInsideViewSpace(const glm::vec2& v)
	{
		constexpr float padding = 1.0f;
		return !(v.x > 1900 + padding || v.x < 0 - padding || v.y > 1000 + padding || v.y < 0 - padding);
	}

	typedef int OutCode;

	const int INSIDE = 0; // 0000
	const int LEFT = 1;   // 0001
	const int RIGHT = 2;  // 0010
	const int BOTTOM = 4; // 0100
	const int TOP = 8;    // 1000

	OutCode ComputeOutCode(const glm::vec2& p)
	{
		OutCode code = INSIDE;

		if (p.x < 0.0f)
			code |= LEFT;
		else if (p.x > SCREEN_WIDTH)
			code |= RIGHT;
		if (p.y < 0.0f)
			code |= BOTTOM;
		else if (p.y > SCREEN_HEIGHT)
			code |= TOP;

		return code;
	}

	bool IsLineInsideViewSpace(glm::vec2 p0, glm::vec2 p1)
	{
		OutCode outcode0 = ComputeOutCode(p0);
		OutCode outcode1 = ComputeOutCode(p1);
		bool accept = false;

		while (true)
		{
			if (!(outcode0 | outcode1))
			{
				accept = true;
				break;
			}
			else if (outcode0 & outcode1)
			{
				break;
			}
			else
			{
				float x, y;
				OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;
				if (outcodeOut & TOP)
				{
					x = p0.x + (p1.x - p0.x) * (SCREEN_HEIGHT - p0.y) / (p1.y - p0.y);
					y = SCREEN_HEIGHT;
				}
				else if (outcodeOut & BOTTOM)
				{
					x = p0.x + (p1.x - p0.x) * (-p0.y) / (p1.y - p0.y);
					y = 0;
				}
				else if (outcodeOut & RIGHT)
				{
					y = p0.y + (p1.y - p0.y) * (SCREEN_WIDTH - p0.x) / (p1.x - p0.x);
					x = SCREEN_WIDTH;
				}
				else if (outcodeOut & LEFT)
				{
					y = p0.y + (p1.y - p0.y) * (-p0.x) / (p1.x - p0.x);
					x = 0;
				}

				if (outcodeOut == outcode0)
				{
					p0.x = x;
					p0.y = y;
					outcode0 = ComputeOutCode(p0);
				}
				else
				{
					p1.x = x;
					p1.y = y;
					outcode1 = ComputeOutCode(p1);
				}
			}
		}

		return accept;
	}

	bool IsBboxInsideViewSpace(const BoundingBox& bbox)
	{
		glm::vec2 p1 = glm::vec2(bbox.min.x, bbox.max.y);
		glm::vec2 p2 = glm::vec2(bbox.max.x, bbox.min.y);
		return IsLineInsideViewSpace(bbox.min, bbox.max) || IsLineInsideViewSpace(p1, p2);
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

			bool isVisible = IsLineInsideViewSpace(last, p1) || IsLineInsideViewSpace(p1, p2) || IsLineInsideViewSpace(p2, last);
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

			bool isVisible = IsLineInsideViewSpace(last, p1) || IsLineInsideViewSpace(p1, p2) || IsLineInsideViewSpace(p2, p3) || IsLineInsideViewSpace(p3, last);
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
