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
