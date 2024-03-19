#include "Flattening.h"

#include "Renderer/Defs.h"
#include "Renderer/Path.h"

#include <glm/gtx/compatibility.hpp>

namespace SvgRenderer::Flattening {

	static bool IsPointInsideViewSpace(const glm::vec2& v)
	{
		return !(v.x > Globals::WindowWidth || v.x < -1.0f || v.y > Globals::WindowHeight || v.y < -1.0f);
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
		else if (p.x > Globals::WindowWidth)
			code |= RIGHT;
		if (p.y < 0.0f)
			code |= BOTTOM;
		else if (p.y > Globals::WindowHeight)
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
					x = p0.x + (p1.x - p0.x) * (Globals::WindowHeight - p0.y) / (p1.y - p0.y);
					y = Globals::WindowHeight;
				}
				else if (outcodeOut & BOTTOM)
				{
					x = p0.x + (p1.x - p0.x) * (-p0.y) / (p1.y - p0.y);
					y = 0;
				}
				else if (outcodeOut & RIGHT)
				{
					y = p0.y + (p1.y - p0.y) * (Globals::WindowWidth - p0.x) / (p1.x - p0.x);
					x = Globals::WindowWidth;
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

	glm::vec2 ProjectPointOntoScreenBoundary(glm::vec2 point)
	{
		if (Flattening::IsPointInsideViewSpace(point))
		{
			return point;
		}

		if (point.y >= 0 && point.y < Globals::WindowHeight - 1)
		{
			if (point.x < 0)
			{
				return glm::vec2(-1, point.y);
			}
			if (point.x >= Globals::WindowWidth)
			{
				return glm::vec2(Globals::WindowWidth, point.y);
			}
		}

		const float distTop = glm::abs(point.y);
		const float distBottom = glm::abs(point.y - Globals::WindowHeight - 1);
		float xCoord = glm::clamp(point.x, -1.0f, static_cast<float>(Globals::WindowWidth));
		if (distBottom < distTop)
		{
			return glm::vec2(xCoord, Globals::WindowHeight);
		}
		return glm::vec2(xCoord, -1);
	}

	static std::vector<glm::vec2> CalculateClosestDistanceScreenBoundary(const glm::vec2& from, const glm::vec2& to)
	{
		auto isPointOnBoundary = [](glm::vec2 p)
		{
			return (p.x == -1.0f || p.x == Globals::WindowWidth || p.y == -1.0f || p.y == Globals::WindowHeight);
		};

		SR_ASSERT(isPointOnBoundary(from));
		SR_ASSERT(isPointOnBoundary(to));

		std::vector<glm::vec2> path;
		path.push_back(from);

		// If they are on the same line on the boundary
		if ((from.x == -1.0f && to.x == -1.0f) ||
			(from.x == Globals::WindowWidth && to.x == Globals::WindowWidth) ||
			(from.y == -1.0f && to.y == -1.0f) ||
			(from.y == Globals::WindowHeight && to.y == Globals::WindowHeight))
		{
			path.push_back(to);
			return path;
		}

		// If the point 'from' is on the top or bottom boundary
		if (from.y == -1.0f || from.y == Globals::WindowHeight)
		{
			if (to.x > from.x)
			{
				path.push_back(glm::vec2(Globals::WindowWidth, from.y));
			}
			else
			{
				path.push_back(glm::vec2(-1.0f, from.y));
			}

			path.push_back(to);
		}
		else // Point 'from' is on the left or right boundary
		{
			if (to.y > from.y)
			{
				path.push_back(glm::vec2(from.x, Globals::WindowHeight));
			}
			else
			{
				path.push_back(glm::vec2(from.x, -1.0f));
			}
		}

		return path;
	}

	struct Segment
	{
		glm::vec2 p1;
		glm::vec2 p2;
	};

	static bool LineLineIntersection(const Segment& seg1, const Segment& seg2, glm::vec2& intersection)
	{
		const float x1 = seg1.p1.x;
		const float y1 = seg1.p1.y;
		const float x2 = seg1.p2.x;
		const float y2 = seg1.p2.y;

		const float x3 = seg2.p1.x;
		const float y3 = seg2.p1.y;
		const float x4 = seg2.p2.x;
		const float y4 = seg2.p2.y;

		const float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

		if (glm::abs(d) < std::numeric_limits<float>::epsilon())
		{
			return false;
		}

		const float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / d;
		const float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / d;

		if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
		{
			intersection = glm::vec2(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
			return true;
		}

		return false;
	}

	static Segment segLow = Segment{ .p1 = glm::vec2(-1.0f, -1.0f), .p2 = glm::vec2(Globals::WindowWidth, -1.0f) };
	static Segment segUp = Segment{ .p1 = glm::vec2(-1.0f, Globals::WindowHeight), .p2 = glm::vec2(Globals::WindowWidth, Globals::WindowHeight) };
	static Segment segLeft = Segment{ .p1 = glm::vec2(-1.0f, -1.0f), .p2 = glm::vec2(-1.0f, Globals::WindowHeight) };
	static Segment segRight = Segment{ .p1 = glm::vec2(Globals::WindowWidth, -1.0f), .p2 = glm::vec2(Globals::WindowWidth, Globals::WindowHeight) };

	static uint32_t FindIntersectionWithScreen(const Segment& seg, glm::vec2& intersectionNear, glm::vec2& intersectionFar)
	{
		std::array<glm::vec2, 4> ints;

		uint32_t bits = 0;
		bits |= LineLineIntersection(segLow, seg, ints[0]) << 0;
		bits |= LineLineIntersection(segUp, seg, ints[1]) << 1;
		bits |= LineLineIntersection(segLeft, seg, ints[2]) << 2;
		bits |= LineLineIntersection(segRight, seg, ints[3]) << 3;

		if (bits == 0)
		{
			return 0; // This should never happen
		}

		// There will be max 2 intersection with screen bbox and a line segment
		std::array<glm::vec2, 2> result;

		uint32_t count = 0;
		for (uint32_t i = 0; i < 4; i++)
		{
			if (bits & (1 << i))
			{
				result[count++] = ints[i];
			}
		}

		if (count == 1)
		{
			intersectionNear = result[0];
		}
		else
		{
			if (glm::distance(seg.p1, result[0]) < glm::distance(seg.p1, result[1]))
			{
				intersectionNear = result[0];
				intersectionFar = result[1];
			}
			else
			{
				intersectionNear = result[1];
				intersectionFar = result[0];
			}
		}

		return count;
	}

	static bool IsEqual(const glm::vec2& vec1, const glm::vec2& vec2, float tolerance = 0.01f)
	{
		float xd = glm::abs(vec1.x - vec2.x);
		float yd = glm::abs(vec1.y - vec2.y);
		return (glm::abs(vec1.x - vec2.x) <= tolerance) && (glm::abs(vec1.y - vec2.y) <= tolerance);
	}

	static uint32_t HandleLineNumberOfSimpleCommands(glm::vec2 last, glm::vec2 point, bool wasLastMove)
	{
		if (wasLastMove)
		{
			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point) && IsLineInsideViewSpace(last, point))
			{
				glm::vec2 intersectionNear, intersectionFar;
				uint32_t count = FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersectionNear, intersectionFar);
				SR_ASSERT(count == 2);

				return 2;
			}

			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);

				if (IsEqual(lastProjected, pointProjected))
				{
					return 1;
				}

				std::vector<glm::vec2> points = CalculateClosestDistanceScreenBoundary(lastProjected, pointProjected);
				return static_cast<uint32_t>(points.size());
			}
			else if (!IsPointInsideViewSpace(last) && IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(lastProjected, intersection))
				{
					return 2;
				}

				return 3;
			}
			else if (IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, pointProjected))
				{
					return 2;
				}

				return 3;
			}
			else
			{
				if (IsEqual(last, point))
				{
					return 1;
				}

				return 2;
			}
		}
		else
		{
			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point) && IsLineInsideViewSpace(last, point))
			{
				glm::vec2 intersectionNear, intersectionFar;
				uint32_t count = FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersectionNear, intersectionFar);
				SR_ASSERT(count == 2);

				return 2;
			}

			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);

				std::vector<glm::vec2> points = CalculateClosestDistanceScreenBoundary(lastProjected, pointProjected);
				return static_cast<uint32_t>(points.size() - 1);
			}
			else if (!IsPointInsideViewSpace(last) && IsPointInsideViewSpace(point))
			{
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, point))
				{
					return 1;
				}

				return 2;
			}
			else if (IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, pointProjected))
				{
					return 1;
				}

				return 2;
			}
			else
			{
				return 1;
			}
		}
	}

	static void HandleLine(glm::vec2 last, glm::vec2 point, bool wasLastMove, std::vector<SimpleCommand>& simpleCmds)
	{
		if (wasLastMove)
		{
			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point) && IsLineInsideViewSpace(last, point))
			{
				glm::vec2 intersectionNear, intersectionFar;
				uint32_t count = FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersectionNear, intersectionFar);
				SR_ASSERT(count == 2);

				simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = intersectionNear });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersectionFar });
				return;
			}

			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);

				if (IsEqual(lastProjected, pointProjected))
				{
					simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = lastProjected });
					return;
				}

				std::vector<glm::vec2> points = CalculateClosestDistanceScreenBoundary(lastProjected, pointProjected);
				simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = lastProjected });
				for (uint32_t i = 1; i < points.size(); i++)
				{
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = points[i] });
				}
			}
			else if (!IsPointInsideViewSpace(last) && IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(lastProjected, intersection))
				{
					simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = lastProjected });
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
					return;
				}

				simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = lastProjected });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = point });
			}
			else if (IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, pointProjected))
				{
					simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = last });
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
					return;
				}

				simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = last });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = pointProjected });
			}
			else
			{
				if (IsEqual(last, point))
				{
					simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = last });
					return;
				}

				simpleCmds.push_back(SimpleCommand{ .type = MOVE_TO, .point = last });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = point });
			}
		}
		else
		{
			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point) && IsLineInsideViewSpace(last, point))
			{
				glm::vec2 intersectionNear, intersectionFar;
				uint32_t count = FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersectionNear, intersectionFar);
				SR_ASSERT(count == 2);

				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersectionNear });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersectionFar });
				return;
			}

			if (!IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 lastProjected = ProjectPointOntoScreenBoundary(last);
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);

				std::vector<glm::vec2> points = CalculateClosestDistanceScreenBoundary(lastProjected, pointProjected);
				for (uint32_t i = 1; i < points.size(); i++)
				{
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = points[i] });
				}
			}
			else if (!IsPointInsideViewSpace(last) && IsPointInsideViewSpace(point))
			{
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, point))
				{
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
					return;
				}

				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = point });
			}
			else if (IsPointInsideViewSpace(last) && !IsPointInsideViewSpace(point))
			{
				glm::vec2 pointProjected = ProjectPointOntoScreenBoundary(point);
				glm::vec2 intersection;
				FindIntersectionWithScreen(Segment{ .p1 = last, .p2 = point }, intersection, intersection);
				if (IsEqual(intersection, pointProjected))
				{
					simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
					return;
				}

				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = intersection });
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = pointProjected });
			}
			else
			{
				simpleCmds.push_back(SimpleCommand{ .type = LINE_TO, .point = point });
			}
		}
	}

	uint32_t CalculateNumberOfSimpleCommands(uint32_t cmdIndex, glm::vec2 last, float tolerance)
	{
		std::vector<SimpleCommand> simpleCmds;

		const PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex];
		uint32_t cmdType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		if (cmdType == MOVE_TO)
		{
			return 0;
		}

		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd.pathIndexCmdType);
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		bool wasLastMove = false;
		if (path.startCmdIndex == cmdIndex || GET_CMD_TYPE(Globals::AllPaths.commands[cmdIndex - 1].pathIndexCmdType) == MOVE_TO)
		{
			wasLastMove = true;
		}

		switch (cmdType)
		{
		case MOVE_TO:
			SR_ASSERT(false);
			break;
		case LINE_TO:
		{
			glm::vec2 point = cmd.transformedPoints[0];
			return HandleLineNumberOfSimpleCommands(last, point, wasLastMove);
		}
		case QUAD_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];

			glm::vec2 lastFLattened = last;

			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * p1 + p2)));
			float t = 0.0f;
			uint32_t count = 0;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 point = glm::lerp(p01, p12, t);

				count += HandleLineNumberOfSimpleCommands(lastFLattened, point, wasLastMove);
				lastFLattened = point;
				wasLastMove = false;
			}

			return count;
		}
		case CUBIC_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];
			const glm::vec2& p3 = cmd.transformedPoints[2];

			glm::vec2 lastFLattened = last;

			const glm::vec2 a = -1.0f * last + 3.0f * p1 - 3.0f * p2 + p3;
			const glm::vec2 b = 3.0f * (last - 2.0f * p1 + p2);
			const float conc = glm::max(glm::length(b), glm::length(a + b));
			const float dt = glm::sqrt((glm::sqrt(8.0f) * tolerance) / conc);

			float t = 0.0f;
			uint32_t count = 0;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 p23 = glm::lerp(p2, p3, t);
				const glm::vec2 p012 = glm::lerp(p01, p12, t);
				const glm::vec2 p123 = glm::lerp(p12, p23, t);
				const glm::vec2 point = glm::lerp(p012, p123, t);

				count += HandleLineNumberOfSimpleCommands(lastFLattened, point, wasLastMove);
				lastFLattened = point;
				wasLastMove = false;
			}

			return count;
		}
		default:
			SR_ASSERT(false, "Unknown path type");
			break;
		}

		return 0;
	}

	std::vector<SvgRenderer::SimpleCommand> Flatten(uint32_t cmdIndex, glm::vec2 last, float tolerance)
	{
		std::vector<SimpleCommand> simpleCmds;

		const PathRenderCmd& cmd = Globals::AllPaths.commands[cmdIndex];
		uint32_t cmdType = GET_CMD_TYPE(cmd.pathIndexCmdType);
		if (cmdType == MOVE_TO)
		{
			return {};
		}

		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd.pathIndexCmdType);
		const PathRender& path = Globals::AllPaths.paths[pathIndex];
		bool wasLastMove = false;
		if (path.startCmdIndex == cmdIndex || GET_CMD_TYPE(Globals::AllPaths.commands[cmdIndex - 1].pathIndexCmdType) == MOVE_TO)
		{
			wasLastMove = true;
		}

		switch (cmdType)
		{
		case MOVE_TO:
			SR_ASSERT(false, "Should not get here");
			break;
		case LINE_TO:
		{
			glm::vec2 point = cmd.transformedPoints[0];
			HandleLine(last, point, wasLastMove, simpleCmds);
			break;
		}
		case QUAD_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];

			glm::vec2 lastFlattened = last;

			const float dt = glm::sqrt(((4.0f * tolerance) / glm::length(last - 2.0f * p1 + p2)));
			float t = 0.0f;
			while (t < 1.0f)
			{
				t = glm::min(t + dt, 1.0f);
				const glm::vec2 p01 = glm::lerp(last, p1, t);
				const glm::vec2 p12 = glm::lerp(p1, p2, t);
				const glm::vec2 point = glm::lerp(p01, p12, t);

				HandleLine(lastFlattened, point, wasLastMove, simpleCmds);
				lastFlattened = point;
				wasLastMove = false;
			}

			break;
		}
		case CUBIC_TO:
		{
			const glm::vec2& p1 = cmd.transformedPoints[0];
			const glm::vec2& p2 = cmd.transformedPoints[1];
			const glm::vec2& p3 = cmd.transformedPoints[2];

			glm::vec2 lastFlattened = last;

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
				const glm::vec2 point = glm::lerp(p012, p123, t);

				HandleLine(lastFlattened, point, wasLastMove, simpleCmds);
				lastFlattened = point;
				wasLastMove = false;
			}

			break;
		}
		default:
			SR_ASSERT(false, "Unknown path type");
			break;
		}

		return simpleCmds;
	}

}
