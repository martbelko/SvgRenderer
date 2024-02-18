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

}
