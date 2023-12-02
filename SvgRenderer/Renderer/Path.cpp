#include "Path.h"

#include <glm/gtx/compatibility.hpp>

#include <cassert>

namespace SvgRenderer {

	static glm::vec2 ApplyTransform(const glm::mat3& transform, const glm::vec2& point)
	{
		glm::vec2 res;
		res = glm::mat2(transform) * glm::vec3(point, 0.0f) + glm::vec2(324.90716f, 255.00942f);
		SR_TRACE("Transform ({0};{1}) into ({2};{3})", point.x, point.y, res.x, res.y);
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
			break;
		case PathCmdType::Close:
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

	/*
	#[inline]
	fn offset(path: &mut Vec<PathCmd>, width: f32, contour: &[PathCmd], closed: bool, reverse: bool) {
		let first_point = if closed == reverse {
			get_point(contour[0])
		} else {
			get_point(*contour.last().unwrap())
		};
		let mut prev_point = first_point;
		let mut prev_normal = Vec2::new(0.0, 0.0);
		let mut i = 0;
		loop {
			let next_point = if i < contour.len() {
				if reverse {
					get_point(contour[contour.len() - i - 1])
				} else {
					get_point(contour[i])
				}
			} else {
				first_point
			};

			if next_point != prev_point || i == contour.len() {
				let next_tangent = next_point - prev_point;
				let next_normal = Vec2::new(-next_tangent.y, next_tangent.x);
				let next_normal_len = next_normal.length();
				let next_normal = if next_normal_len == 0.0 {
					Vec2::new(0.0, 0.0)
				} else {
					next_normal * (1.0 / next_normal_len)
				};

				join(path, width, prev_normal, next_normal, prev_point);

				prev_point = next_point;
				prev_normal = next_normal;
			}

			i += 1;
			if i > contour.len() {
				break;
			}
		}
	}

	let mut output = Vec::new();

	let mut contour_start = 0;
	let mut contour_end = 0;
	let mut closed = false;
	let mut commands = polygon.iter();
	loop {
		let command = commands.next();

		if let Some(PathCmd::Close) = command {
			closed = true;
		}

		if let None | Some(PathCmd::Move(_)) | Some(PathCmd::Close) = command {
			if contour_start != contour_end {
				let contour = &polygon[contour_start..contour_end];

				let base = output.len();
				offset(&mut output, width, contour, closed, false);
				output[base] = PathCmd::Move(get_point(output[base]));
				if closed {
					output.push(PathCmd::Close);
				}

				let base = output.len();
				offset(&mut output, width, contour, closed, true);
				if closed {
					output[base] = PathCmd::Move(get_point(output[base]));
				}
				output.push(PathCmd::Close);
			}
		}

		if let Some(command) = command {
			match command {
				PathCmd::Move(_) => {
					contour_start = contour_end;
					contour_end = contour_start + 1;
				}
				PathCmd::Line(_) => {
					contour_end += 1;
				}
				PathCmd::Close => {
					contour_start = contour_end + 1;
					contour_end = contour_start;
					closed = true;
				}
				_ => {
					panic!();
				}
			}
		} else {
			break;
		}
	}

	output
}
	*/

}
