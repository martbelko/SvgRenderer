#include "Path.h"

#include <glm/gtx/compatibility.hpp>

#include <cassert>

namespace SvgRenderer {

	static glm::vec2 ApplyTransform(const glm::mat3& transform, const glm::vec2& point)
	{
		glm::vec2 res = transform * glm::vec3(point, 1.0f);
		//SR_TRACE("Transform ({0};{1}) into ({2};{3})", point.x, point.y, res.x, res.y);
		return res;
	}

	PathCmd PathCmd::Transform(const glm::mat3& transform) const
	{
		switch (type)
		{
		case PathCmdType::MoveTo:
			return PathCmd(MoveToCmd(ApplyTransform(transform, as.moveTo.point)));
		case PathCmdType::LineTo:
			return PathCmd(LineToCmd(ApplyTransform(transform, as.lineTo.p1)));
		case PathCmdType::QuadTo:
			return PathCmd(QuadToCmd(ApplyTransform(transform, as.quadTo.p1), ApplyTransform(transform, as.quadTo.p2)));
		case PathCmdType::CubicTo:
			return PathCmd(CubicToCmd(ApplyTransform(transform, as.cubicTo.p1), ApplyTransform(transform, as.cubicTo.p2), ApplyTransform(transform, as.cubicTo.p3)));
		case PathCmdType::ConicTo:
			// TODO: Implement
			assert(false && "Not implemented");
			break;
		case PathCmdType::Close:
			return PathCmd(CloseCmd{});
		}

		assert(false && "Unknown path type");
		return PathCmd(CloseCmd{});
	}

	void PathCmd::Flatten(glm::vec2 last, float tolerance, std::function<void(const PathCmd&)> callback) const
	{
		switch (type)
		{
		case PathCmdType::MoveTo:
			callback(MoveToCmd(as.moveTo.point));
			break;
		case PathCmdType::LineTo:
			callback(LineToCmd(as.lineTo.p1));
			break;
		case PathCmdType::QuadTo:
		{
			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * as.quadTo.p1 + as.quadTo.p2)));
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, as.quadTo.p1, t);
				const glm::vec2 p12 = glm::lerp(as.quadTo.p1, as.quadTo.p2, t);
				callback(LineToCmd{ .p1 = glm::lerp(p01, p12, t) });
			}

			break;
		}
		case PathCmdType::CubicTo:
		{
			const glm::vec2 a = -1.0f * last + 3.0f * as.cubicTo.p1 - 3.0f * as.cubicTo.p2 + as.cubicTo.p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * as.cubicTo.p1 + as.cubicTo.p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, as.cubicTo.p1, t);
				const glm::vec2 p12 = glm::lerp(as.cubicTo.p1, as.cubicTo.p2, t);
				const glm::vec2 p23 = glm::lerp(as.cubicTo.p2, as.cubicTo.p3, t);
				const glm::vec2 p012 = glm::lerp(p01, p12, t);
				const glm::vec2 p123 = glm::lerp(p12, p23, t);
				callback(LineToCmd{ .p1 = glm::lerp(p012, p123, t) });
			}

			break;
		}
		case PathCmdType::ConicTo:
			// TODO: Implement
			assert(false && "Not implemented");
			break;
		case PathCmdType::Close:
			// TODO: Implement
			assert(false && "Not implemented");
			break;
		default:
			assert(false && "Unknown path type");
			break;
		}
	}

	std::vector<PathCmd> PathFlatten(const std::vector<PathCmd>& commands, float tolerance)
	{
		glm::vec2 last = { 0.0f, 0.0f };
		std::vector<PathCmd> result;

		for (const PathCmd& cmd : commands)
		{
			cmd.Flatten(last, tolerance, [&](const PathCmd& cmd)
			{
				result.push_back(cmd);
			});

			switch (cmd.type)
			{
			case PathCmdType::MoveTo:
				last = cmd.as.moveTo.point;
				break;
			case PathCmdType::LineTo:
				last = cmd.as.lineTo.p1;
				break;
			case PathCmdType::QuadTo:
				last = cmd.as.quadTo.p2;
				break;
			case PathCmdType::CubicTo:
				last = cmd.as.cubicTo.p3;
				break;
			case PathCmdType::ConicTo:
				// TODO: Implement
				break;
			case PathCmdType::Close:
				break;
			}
		}

		return result;
	}

	std::vector<PathCmd> PathStroke(const std::vector<PathCmd>& polygon, float width)
	{
		// TODO: Finish
		return {};

		auto getPoint = [](const PathCmd& cmd) -> glm::vec2
		{
			switch (cmd.type)
			{
			case PathCmdType::MoveTo:
				return cmd.as.moveTo.point;
			case PathCmdType::LineTo:
				return cmd.as.lineTo.p1;
			}

			assert(false && "Only lines or moves");
			return cmd.as.moveTo.point;
		};

		auto join = [](std::vector<PathCmd>& path, float width, const glm::vec2& prevNormal, const glm::vec2& nextNormal, const glm::vec2& point)
			{
				float offset = 1.0f / (1.0f + glm::dot(prevNormal, nextNormal));
				if (glm::abs(offset) > 2.0f)
				{
					path.push_back(LineToCmd(point + 0.5f * width * prevNormal));
					path.push_back(LineToCmd(point + 0.5f * width * nextNormal));
				}
				else
				{
					path.push_back(LineToCmd(point + 0.5f * width * offset * (prevNormal + nextNormal)));
				}
			};
	}

}
