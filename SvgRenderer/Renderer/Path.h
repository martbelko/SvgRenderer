#pragma once

#include <glm/glm.hpp>

namespace SvgRenderer {

	// TODO: Add arc
	enum class PathCmdType
	{
		MoveTo = 0,
		LineTo,
		QuadTo,
		CubicTo,
		ConicTo,
		Close
	};

	struct MoveToCmd
	{
		glm::vec2 point;
	};

	struct LineToCmd
	{
		glm::vec2 p1;
	};

	struct QuadToCmd
	{
		glm::vec2 p1;
		glm::vec2 p2;
	};

	struct CubicToCmd
	{
		glm::vec2 p1;
		glm::vec2 p2;
		glm::vec2 p3;
	};

	struct CloseCmd
	{
	};

	struct PathCmd
	{
		PathCmdType type;
		union CmdAs
		{
			MoveToCmd moveTo;
			LineToCmd lineTo;
			QuadToCmd quadTo;
			CubicToCmd cubicTo;
			CloseCmd close;
		} as;

		PathCmd(const MoveToCmd& cmd)
			: type(PathCmdType::MoveTo), as(CmdAs{ .moveTo = cmd }) {}
		PathCmd(const LineToCmd& cmd)
			: type(PathCmdType::LineTo), as(CmdAs{ .lineTo = cmd }) {}
		PathCmd(const QuadToCmd& cmd)
			: type(PathCmdType::QuadTo), as(CmdAs{ .quadTo = cmd }) {}
		PathCmd(const CubicToCmd& cmd)
			: type(PathCmdType::CubicTo), as(CmdAs{ .cubicTo = cmd }) {}
		PathCmd(const CloseCmd& cmd)
			: type(PathCmdType::Close), as(CmdAs{ .close = cmd }) {}

		PathCmd Transform(const glm::mat3& transform) const;
		void Flatten(glm::vec2 last, float tolerance, std::function<void(const PathCmd&)> callback) const;
	};

	std::vector<PathCmd> PathFlatten(const std::vector<PathCmd>& commands, float tolerance);
	std::vector<PathCmd> PathStroke(const std::vector<PathCmd>& polygon, float width);

}
