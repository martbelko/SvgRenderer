#include "Application.h"

#include "Core/Filesystem.h"
#include "Core/SvgParser.h"
#include "Core/Timer.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/TileBuilder.h"
#include "Renderer/Rasterizer.h"
#include "Renderer/StorageBuffer.h"
#include "Renderer/UniformBuffer.h"

#include "Renderer/Defs.h"
#include "Renderer/Flattening.h"
#include "Renderer/Bezier.h"

#include "Scene/OrthographicCamera.h"

#include "Utils/BoundingBox.h"
#include "Utils/Equations.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <execution>
#include <future>

namespace SvgRenderer {

	uint32_t g_PathIndex = 0;

	Application Application::s_Instance;

	static std::pair<QuadraticBezier, QuadraticBezier> SplitByClosestPoint(const QuadraticBezier& quadBez)
	{
		BezierPoint splitPoint = quadBez.GetClosestPointToControlPoint();
		glm::vec2 cp1 = (1.0f - splitPoint.t) * quadBez.p0 + splitPoint.t * quadBez.p1;
		glm::vec2 cp2 = (1.0f - splitPoint.t) * quadBez.p1 + splitPoint.t * quadBez.p2;

		QuadraticBezier bez1{
			.p0 = quadBez.p0,
			.p1 = cp1,
			.p2 = splitPoint.point
		};

		QuadraticBezier bez2{
			.p0 = splitPoint.point,
			.p1 = cp2,
			.p2 = quadBez.p2
		};

		return std::make_pair(std::move(bez1), std::move(bez2));
	}

	static std::array<CubicBezier, 3> SplitByClosestPoints(const CubicBezier& cubicBez)
	{
		const BezierPoint s1 = cubicBez.GetClosestPointToControlPoint(0);
		const BezierPoint s2 = cubicBez.GetClosestPointToControlPoint(1);
		const float a = s1.t;
		const float b = s2.t;

		glm::vec2 p_000 = cubicBez.p0;
		glm::vec2 p_001 = cubicBez.p1;
		glm::vec2 p_011 = cubicBez.p2;
		glm::vec2 p_111 = cubicBez.p3;

		glm::vec2 p_00a = (1 - a) * p_000 * a * p_001;
		glm::vec2 p_00b = (1 - b) * p_000 * b * p_001;

		glm::vec2 p_01a = (1 - a) * p_001 + a * p_011;
		glm::vec2 p_01b = (1 - b) * p_001 + b * p_011;

		glm::vec2 p_0aa = (1 - a) * p_00a + a * p_01a;
		glm::vec2 p_0bb = (1 - b) * p_00b + b * p_01b;

		glm::vec2 p_0ab = (1 - a) * p_00b + a * p_0bb;

		glm::vec2 p_a11 = (1 - a) * p_011 + a * p_111;
		glm::vec2 p_b11 = (1 - b) * p_011 + b * p_111;

		glm::vec2 p_1ab = (1 - a) * p_01b + a * p_b11;
		glm::vec2 p_1bb = (1 - b) * p_01b + b * p_b11;

		glm::vec2 p_bbb = (1 - b) * p_0bb + b * p_1bb;
		// glm::vec2 p_aaa = (1 - a) * p_0aa + a * p_1aa;

		auto printvec = [](const glm::vec2& v)
		{
			std::cout << v.x << ' ' << v.y << ' ';
		};
		printvec(p_bbb);
		printvec(p_1bb);
		printvec(p_b11);
		printvec(p_111);

		return std::array<CubicBezier, 3>();
	}

	static void CreateCommandsFromQuadBeziers(std::vector<PathCmd>& cmds, const std::vector<glm::vec2>& points, float halfWidth)
	{
		float d = halfWidth;
		bool fp = true;

		std::vector<glm::vec2> ps1;
		std::vector<glm::vec2> ps2;

		for (uint32_t i = 0; i < points.size() - 1; i += 2)
		{
			glm::vec2 cp1 = points[i + 0];
			glm::vec2 cp2 = points[i + 1];
			glm::vec2 cp3 = points[i + 2];

			glm::vec2 n1 = -glm::normalize(QuadraticBezier::EvaluateDerivative(cp1, cp2, cp3, 0.0f));
			glm::vec2 n3 = -glm::normalize(QuadraticBezier::EvaluateDerivative(cp1, cp2, cp3, 1.0f));
			n1 = { -n1.y, n1.x };
			n3 = { -n3.y, n3.x };
			glm::vec2 n2 = n1 + n3;

			float k = 2.0f * d / glm::length(n2);
			glm::vec2 kvec = k * glm::normalize(n2);

			glm::vec2 q1 = cp1 + d * n1;
			glm::vec2 q2 = cp2 + kvec;
			glm::vec2 q3 = cp3 + d * n3;

			glm::vec2 p1 = cp1 - d * n1;
			glm::vec2 p2 = cp2 - kvec;
			glm::vec2 p3 = cp3 - d * n3;

			if (fp)
			{
				fp = false;
				ps1.push_back(q1);
				ps2.push_back(p1);
			}

			ps1.push_back(q2);
			ps1.push_back(q3);
			ps2.push_back(p2);
			ps2.push_back(p3);
		}

		cmds.push_back(PathCmd(MoveToCmd{ .point = ps1[0] }));
		for (uint32_t i = 1; i < ps1.size() - 1; i += 2)
		{
			cmds.push_back(PathCmd(QuadToCmd{ .p1 = ps1[i], .p2 = ps1[i + 1] }));
		}

		cmds.push_back(PathCmd(LineToCmd{ .p1 = ps2[ps2.size() - 1] }));
		for (uint32_t i = ps2.size() - 1; i > 0; i -= 2)
		{
			cmds.push_back(PathCmd(QuadToCmd{ .p1 = ps2[i - 1], .p2 = ps2[i - 2] }));
		}

		cmds.push_back(PathCmd(LineToCmd{ .p1 = ps1[0] }));
	}

	static void AddStroke(std::vector<PathCmd>& cmds, const QuadraticBezier& quadBez, float width)
	{
		const uint32_t iterations = 3;

		std::vector<glm::vec2> points;
		uint32_t count = 1 + glm::pow(2, iterations + 1);
		points.resize(count);

		points[0] = (quadBez.p0);
		points[count / 2] = (quadBez.p1);
		points[count - 1] = (quadBez.p2);

		uint32_t bezCount = 1;
		uint32_t step = count - 1;
		for (uint32_t i = 0; i < iterations; i++)
		{
			for (uint32_t j = 0; j < bezCount; j++)
			{
				// 0 -> [0, 8]
				// 1 -> [0, 4], [4, 8]
				// 2 -> [0, 2], [2, 4], [4, 6], [6, 8]
				uint32_t low = j * step;
				uint32_t high = low + step;
				uint32_t mid = (low + high) / 2;

				QuadraticBezier qb{
					.p0 = points[low],
					.p1 = points[mid],
					.p2 = points[high]
				};

				auto [sqb1, sqb2] = SplitByClosestPoint(qb);

				uint32_t mid1 = (low + mid) / 2;
				uint32_t mid2 = (mid + high) / 2;

				points[mid1] = sqb1.p1;
				points[mid] = sqb1.p2;
				points[mid2] = sqb2.p1;
			}

			bezCount *= 2;
			step /= 2;
		}

		CreateCommandsFromQuadBeziers(cmds, points, width / 2.0f);
	}

	static void AddStroke(std::vector<PathCmd>& cmds, const glm::vec2& from, const glm::vec2& to, float width)
	{
		const float halfWidth = width / 2.0f;

		glm::vec2 dir = glm::normalize(to - from);
		glm::vec2 n = { -dir.y, dir.x };

		glm::vec2 p1 = from - halfWidth * n;
		glm::vec2 p2 = to - halfWidth * n;
		glm::vec2 p3 = to + halfWidth * n;
		glm::vec2 p4 = from + halfWidth * n;

		cmds.push_back(PathCmd(MoveToCmd{ .point = p1 }));
		cmds.push_back(PathCmd(LineToCmd{ .p1 = p2 }));
		cmds.push_back(PathCmd(LineToCmd{ .p1 = p3 }));
		cmds.push_back(PathCmd(LineToCmd{ .p1 = p4 }));
		cmds.push_back(PathCmd(LineToCmd{ .p1 = p1 }));
	}

	static void AddFillPath(const SvgPath& path, TileBuilder& builder)
	{
		glm::vec2 first = { 0, 0 };
		glm::vec2 last = { 0, 0 };

		std::vector<PathCmd> cmds;
		for (const SvgPath::Segment& seg : path.segments)
		{
			switch (seg.type)
			{
			case SvgPath::Segment::Type::MoveTo:
			{
				if (last != first)
				{
					cmds.push_back(PathCmd(LineToCmd{ .p1 = first }));
				}

				cmds.push_back(PathCmd(MoveToCmd{ .point = seg.as.moveTo.p }));
				first = seg.as.moveTo.p;
				last = seg.as.moveTo.p;
				break;
			}
			case SvgPath::Segment::Type::LineTo:
				if (seg.as.lineTo.p != last)
				{
					cmds.push_back(PathCmd(LineToCmd{ .p1 = seg.as.lineTo.p }));
				}

				last = seg.as.lineTo.p;
				break;
			case SvgPath::Segment::Type::Close:
				if (first != last)
				{
					cmds.push_back(LineToCmd{ .p1 = first });
				}

				last = first;
				break;
			case SvgPath::Segment::Type::QuadTo:
				cmds.push_back(PathCmd(QuadToCmd{ .p1 = seg.as.quadTo.p1, .p2 = seg.as.quadTo.p2 }));
				last = seg.as.quadTo.p2;
				break;
			case SvgPath::Segment::Type::CubicTo:
				cmds.push_back(PathCmd(CubicToCmd{ .p1 = seg.as.cubicTo.p1, .p2 = seg.as.cubicTo.p2, .p3 = seg.as.cubicTo.p3 }));
				last = seg.as.cubicTo.p3;
				break;
			// TODO: Implement others
			}
		}

		// Only for FILL, not for STROKE
		if (last != first)
		{
			cmds.push_back(PathCmd(LineToCmd{ .p1 = first }));
		}

		const SvgColor& fillColor = path.fill.color;
		std::array<uint8_t, 4> color = { fillColor.r, fillColor.g, fillColor.b, static_cast<uint8_t>(path.fill.opacity * 255.0f) };

		Globals::AllPaths.paths.push_back(PathRender{
			.startCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size()),
			.endCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size() + cmds.size() - 1),
			.transform = path.transform,
			.bbox = BoundingBox(),
			.color = color
			});

		for (const PathCmd& cmd : cmds)
		{
			uint32_t index = Globals::AllPaths.paths.size() - 1; // -1, since we added path in the previous lines
			uint32_t pathIndexCmdType = MAKE_CMD_PATH_INDEX(0, index);
			pathIndexCmdType = MAKE_CMD_TYPE(pathIndexCmdType, static_cast<uint32_t>(cmd.type));

			// This is just to fill all the 3 points, even though not all may be used
			// It is based on the cmd type
			std::array<glm::vec2, 4> points; // We use only 3 of 4, it is just for padding
			points[0] = cmd.as.cubicTo.p1;
			points[1] = cmd.as.cubicTo.p2;
			points[2] = cmd.as.cubicTo.p3;

			Globals::AllPaths.commands.push_back(PathRenderCmd{
				.pathIndexCmdType = pathIndexCmdType,
				.points = points
				});
		}
	}

	static void AddStrokePath(const SvgPath& path, TileBuilder& builder)
	{
		glm::vec2 first = { 0, 0 };
		glm::vec2 last = { 0, 0 };

		std::vector<PathCmd> cmds;
		for (const SvgPath::Segment& seg : path.segments)
		{
			switch (seg.type)
			{
			case SvgPath::Segment::Type::MoveTo:
			{
				first = seg.as.moveTo.p;
				last = seg.as.moveTo.p;
				break;
			}
			case SvgPath::Segment::Type::LineTo:
				if (seg.as.lineTo.p != first)
				{
					AddStroke(cmds, last, seg.as.lineTo.p, path.stroke.width);
				}

				last = seg.as.lineTo.p;
				break;
			case SvgPath::Segment::Type::Close:
				if (last != first)
				{
					AddStroke(cmds, last, first, path.stroke.width);
				}

				last = first;
				break;
			case SvgPath::Segment::Type::QuadTo:
				AddStroke(cmds, QuadraticBezier(last, seg.as.quadTo.p1, seg.as.quadTo.p2), path.stroke.width);
				last = seg.as.quadTo.p2;
				break;
			case SvgPath::Segment::Type::CubicTo:
				last = seg.as.cubicTo.p3;
				break;
			// TODO: Implement others
			}
		}

		const SvgColor& fillColor = path.fill.color;
		std::array<uint8_t, 4> color = { fillColor.r, fillColor.g, fillColor.b, static_cast<uint8_t>(path.fill.opacity * 255.0f) };

		Globals::AllPaths.paths.push_back(PathRender{
			.startCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size()),
			.endCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size() + cmds.size() - 1),
			.transform = path.transform,
			.bbox = BoundingBox(),
			.color = color
		});

		for (const PathCmd& cmd : cmds)
		{
			uint32_t index = Globals::AllPaths.paths.size() - 1; // -1, since we added path in the previous lines
			uint32_t pathIndexCmdType = MAKE_CMD_PATH_INDEX(0, index);
			pathIndexCmdType = MAKE_CMD_TYPE(pathIndexCmdType, static_cast<uint32_t>(cmd.type));

			// This is just to fill all the 3 points, even though not all may be used
			// It is based on the cmd type
			std::array<glm::vec2, 4> points; // We use only 3 of 4 (padding)
			points[0] = cmd.as.cubicTo.p1;
			points[1] = cmd.as.cubicTo.p2;
			points[2] = cmd.as.cubicTo.p3;

			Globals::AllPaths.commands.push_back(PathRenderCmd{
				.pathIndexCmdType = pathIndexCmdType,
				.points = points
			});
		}
	}

	static void Render(const SvgNode* node, TileBuilder& builder)
	{
		switch (node->type)
		{
		case SvgNodeType::Path:
		{
			const SvgPath& path = node->as.path;

			AddFillPath(path, builder);

			if (path.stroke.hasStroke)
			{
				//AddStrokePath(path, builder);
			}

			g_PathIndex++;
			break;
		}
		}

		for (const SvgNode* child : node->children)
		{
			Render(child, builder);
		}
	}

	static glm::vec2 ApplyTransform(const glm::mat4& transform, const glm::vec2& point)
	{
		return Globals::GlobalTransform * (transform * glm::vec4(point, 1.0f, 1.0f));
	}

	static glm::vec2 ApplyTransform(const glm::mat3& transform, const glm::vec2& point)
	{
		return Globals::GlobalTransform * glm::vec4(transform * glm::vec3(point, 1.0f), 1.0f);
	}

	static void TransformCurve(PathRenderCmd* cmd)
	{
		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd->pathIndexCmdType);
		cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
		cmd->transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[2]);

		//uint32_t cmdType = GET_CMD_TYPE(cmd->pathIndexCmdType);
		//switch (cmdType)
		//{
		//case MOVE_TO:
		//case LINE_TO:
		//{
		//	cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		//	break ;
		//}
		//case QUAD_TO:
		//{
		//	cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		//	cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
		//	break;
		//}
		//case CUBIC_TO:
		//{
		//	cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		//	cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
		//	cmd->transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[2]);
		//	break;
		//}
		//}
	};

	void Application::Init()
	{
		uint32_t initWidth = SCREEN_WIDTH, initHeight = SCREEN_HEIGHT;

		m_Window = Window::Create({
			.width = initWidth,
			.height = initHeight,
			.title = "SvgRenderer",
			.callbacks = {
				.onWindowClose = Application::OnWindowCloseStatic,
				.onKeyPressed = Application::OnKeyPressedStatic,
				.onKeyReleased = Application::OnKeyReleasedStatic,
				.onMousePressed = Application::OnMousePressedStatic,
				.onMouseReleased = Application::OnMouseReleasedStatic,
				.onViewportSizeChanged = Application::OnViewportResizeStatic
			}
		});

		CubicBezier cb;
		cb.p0 = { 70, 550 };
		cb.p1 = { 250, 300 };
		cb.p2 = { 500, 350 };
		cb.p3 = { 620, 550 };

		//SplitByClosestPoints(cb);

		Renderer::Init(initWidth, initHeight);

		SR_TRACE("Parsing start");
		SvgNode* root = SvgParser::Parse("C:/Users/Martin/Desktop/svgs/paris.svg");
		SR_TRACE("Parsing finish");

		// This actually fills information about colors and other attributes from the SVG root node
		Render(root, m_TileBuilder);
		delete root;

		// Everything after this line may be done in each frame

#define ASYNC 1
		// 1.step: Transform the paths
#if ASYNC == 1
		//GLuint buf, buf1;
		//glCreateBuffers(1, &buf);
		//glCreateBuffers(1, &buf1);
		//
		//GLenum bufferFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_READ_BIT;
		//glNamedBufferStorage(buf, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data(), bufferFlags);
		//glNamedBufferStorage(buf1, Globals::AllPaths.paths.size() * sizeof(PathRender), Globals::AllPaths.paths.data(), bufferFlags);
		//
		//// glNamedBufferData(buf, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data(), GL_STREAM_DRAW);
		//assert(glGetError() == GL_NO_ERROR);
		//
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf1);
		//assert(glGetError() == GL_NO_ERROR);
		//
		//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf);
		//assert(glGetError() == GL_NO_ERROR);
		//
		//Ref<Shader> shader = Shader::CreateCompute(Filesystem::AssetsPath() / "shaders" / "Test.comp");
		//assert(glGetError() == GL_NO_ERROR);
		//
		//Timer globalTimer;
		//
		//Timer tsTimer;
		//
		//shader->Bind();
		//shader->SetUniformMat4(0, Globals::GlobalTransform);
		//
		//uint32_t ySize = glm::max(glm::ceil(Globals::AllPaths.commands.size() / 65535.0f), 1.0f);
		//shader->Dispatch(65535, ySize, 1);
		//GLenum xx = glGetError();
		//assert(glGetError() == GL_NO_ERROR);
		//
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		//
		//SR_INFO("Transforming paths: {0} ms", tsTimer.ElapsedMillis());
		//tsTimer.Reset();
		//// glFinish();
		//
		//glGetNamedBufferSubData(buf, 0, Globals::AllPaths.commands.size() * sizeof(PathRenderCmd), Globals::AllPaths.commands.data());
		//SR_INFO("Retrieving data GPU -> CPU: {0} ms", tsTimer.ElapsedMillis());
		//
		//glDeleteBuffers(1, &buf);
		//glDeleteBuffers(1, &buf1);

		Timer globalTimer;
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.commands.size());
			std::iota(indices.begin(), indices.end(), 0);

			Timer tsTimer;
			std::for_each(std::execution::par, indices.begin(), indices.end(), [](uint32_t cmdIndex)
			{
				TransformCurve(&Globals::AllPaths.commands[cmdIndex]);
			});
			SR_INFO("Transforming paths: {0} ms", tsTimer.ElapsedMillis());
		}
#else
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); ++pathIndex)
		{
			TransformPath(pathIndex);
		}
#endif
		// 2.step: Calculate number of simple commands and their indexes for each path and each command in the path
#if ASYNC == 1
		auto getPreviousPoint = [](const PathRender& path, uint32_t index) -> glm::vec2
		{
			if (index == path.startCmdIndex)
			{
					return glm::vec2(0, 0);
			}

			PathRenderCmd& rndCmd = Globals::AllPaths.commands[index - 1];
			uint32_t pathType = GET_CMD_TYPE(rndCmd.pathIndexCmdType);
			switch (pathType)
			{
			case MOVE_TO:
				return rndCmd.transformedPoints[0];
			case LINE_TO:
				return rndCmd.transformedPoints[0];
			case QUAD_TO:
				return rndCmd.transformedPoints[1];
			case CUBIC_TO:
				return rndCmd.transformedPoints[2];
			}
		};

		uint32_t simpleCommandsCount = 0;
		{
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			constexpr uint32_t wgSize = 256;
			std::array<uint32_t, wgSize> wgIndices;
			std::iota(wgIndices.begin(), wgIndices.end(), 0);

			std::for_each(std::execution::par, indices.begin(), indices.end(), [&getPreviousPoint, &wgIndices, &wgSize](uint32_t pathIndex)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t offsetCmdIndex = path.startCmdIndex; offsetCmdIndex <= path.endCmdIndex; offsetCmdIndex += wgSize)
				{
					std::for_each(std::execution::par, wgIndices.cbegin(), wgIndices.cend(), [&path, &offsetCmdIndex, &getPreviousPoint](uint32_t wgIndex)
					{
						uint32_t cmdIndex = wgIndex + offsetCmdIndex;
						if (cmdIndex > path.endCmdIndex)
						{
							return;
						}

						PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
						glm::vec2 last = getPreviousPoint(path, cmdIndex);
						uint32_t count = Flattening::CalculateNumberOfSimpleCommands(rndCmd, last, TOLERANCE);
						rndCmd.startIndexSimpleCommands = count;
					});
				}
			});

			for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
				{
					PathRenderCmd& rndCmd = Globals::AllPaths.commands[i];
					uint32_t count = rndCmd.startIndexSimpleCommands;
					rndCmd.startIndexSimpleCommands = simpleCommandsCount;
					simpleCommandsCount += count;
					rndCmd.endIndexSimpleCommands = simpleCommandsCount - 1;
				}
			}
		}

		Globals::AllPaths.simpleCommands.resize(simpleCommandsCount);
#else
		uint32_t simpleCommandsCount = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			const PathRender& path = Globals::AllPaths.paths[pathIndex];
			glm::vec2 last = glm::vec2(0, 0);
			for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
			{
				PathRenderCmd& rndCmd = Globals::AllPaths.commands[i];
				uint32_t count = Flattening::CalculateNumberOfSimpleCommands(rndCmd, last, TOLERANCE);

				uint32_t pathType = GET_CMD_TYPE(rndCmd.pathIndexCmdType);
				switch (pathType)
				{
				case MOVE_TO:
					last = rndCmd.transformedPoints[0];
					break;
				case LINE_TO:
					last = rndCmd.transformedPoints[0];
					break;
				case QUAD_TO:
					last = rndCmd.transformedPoints[1];
					break;
				case CUBIC_TO:
					last = rndCmd.transformedPoints[2];
					break;
				}

				rndCmd.startIndexSimpleCommands = simpleCommandsCount;
				simpleCommandsCount += count;
				rndCmd.endIndexSimpleCommands = simpleCommandsCount - 1;
			}
		}

		Globals::AllPaths.simpleCommands.resize(simpleCommandsCount);
#endif
		// 3.step: Simplify the commands and store in the array
#if ASYNC == 1
		{
			// 3.1 Flattening
			std::vector<uint32_t> indices;
			indices.resize(Globals::AllPaths.paths.size());
			std::iota(indices.begin(), indices.end(), 0);

			constexpr uint32_t wgSize = 256;
			std::array<uint32_t, wgSize> wgIndices;
			std::iota(wgIndices.begin(), wgIndices.end(), 0);

			std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [&getPreviousPoint, &wgIndices, &wgSize](uint32_t pathIndex)
			{
				const PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t offsetCmdIndex = path.startCmdIndex; offsetCmdIndex <= path.endCmdIndex; offsetCmdIndex += wgSize)
				{
					std::for_each(std::execution::par, wgIndices.cbegin(), wgIndices.cend(), [&path, &offsetCmdIndex, &getPreviousPoint](uint32_t wgIndex)
					{
						uint32_t cmdIndex = wgIndex + offsetCmdIndex;
						if (cmdIndex > path.endCmdIndex)
						{
							return;
						}

						PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
						glm::vec2 last = getPreviousPoint(path, cmdIndex);
						Flattening::FlattenIntoArray(rndCmd, last, TOLERANCE);
					});
				}
			});

			// 3.1 Calculating BBOX
			std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				for (uint32_t cmdIndex = path.startCmdIndex; cmdIndex <= path.endCmdIndex; cmdIndex++)
				{
					PathRenderCmd& rndCmd = Globals::AllPaths.commands[cmdIndex];
					for (uint32_t i = rndCmd.startIndexSimpleCommands; i <= rndCmd.endIndexSimpleCommands; i++)
					{
						path.bbox.AddPoint(Globals::AllPaths.simpleCommands[i].point);
					}
				}

				path.bbox.AddPadding({ 1.0f, 1.0f });
			});
		}
#else
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			PathRender& path = Globals::AllPaths.paths[pathIndex];

			glm::vec2 last = glm::vec2(0, 0);
			for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
			{
				PathRenderCmd& rndCmd = Globals::AllPaths.commands[i];
				BoundingBox bbox = Flattening::FlattenIntoArray(rndCmd, last, TOLERANCE);
				path.bbox = BoundingBox::Merge(path.bbox, bbox);

				uint32_t pathType = GET_CMD_TYPE(rndCmd.pathIndexCmdType);
				switch (pathType)
				{
				case MOVE_TO:
					last = rndCmd.transformedPoints[0];
					break;
				case LINE_TO:
					last = rndCmd.transformedPoints[0];
					break;
				case QUAD_TO:
					last = rndCmd.transformedPoints[1];
					break;
				case CUBIC_TO:
					last = rndCmd.transformedPoints[2];
					break;
				}
			}

			// Maybe add more padding?
			path.bbox.AddPadding({ 1.0f, 1.0f });
		}
#endif
		// 4.step: The rest
#if ASYNC == 1
		// 4.1 Calculate correct tile indices for each path according to its bounding box
		Globals::Tiles.tiles.reserve(561'367); // This remains fixed for the whole run of the program, may change in the future

		{
			std::atomic_uint32_t tileCount = 0;
			for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsInsideViewSpace(path.bbox.min) && !Flattening::IsInsideViewSpace(path.bbox.max))
				{
					continue;
				}

				// TODO: We are not interested in tiles, where x-coord of the tile is above SCREEN_WIDTH, or y-coord is above SCREEN_HEIGHT
				const int32_t minBboxCoordX = glm::floor(path.bbox.min.x);
				const int32_t minBboxCoordY = glm::floor(path.bbox.min.y);
				const int32_t maxBboxCoordX = glm::ceil(path.bbox.max.x);
				const int32_t maxBboxCoordY = glm::ceil(path.bbox.max.y);

				const int32_t minTileCoordX = glm::floor(static_cast<float>(minBboxCoordX) / TILE_SIZE);
				const int32_t minTileCoordY = glm::floor(static_cast<float>(minBboxCoordY) / TILE_SIZE);
				const int32_t maxTileCoordX = glm::ceil(static_cast<float>(maxBboxCoordX) / TILE_SIZE);
				const int32_t maxTileCoordY = glm::ceil(static_cast<float>(maxBboxCoordY) / TILE_SIZE);

				int32_t m_TileStartX = minTileCoordX;
				int32_t m_TileStartY = minTileCoordY;
				uint32_t m_TileCountX = maxTileCoordX - minTileCoordX + 1;
				uint32_t m_TileCountY = maxTileCoordY - minTileCoordY + 1;

				const uint32_t count = m_TileCountX * m_TileCountY;
				path.startTileIndex = count;

				for (int32_t y = minTileCoordY; y <= maxTileCoordY; y++)
				{
					for (int32_t x = minTileCoordX; x <= maxTileCoordX; x++)
					{
						Globals::Tiles.tiles.push_back(Tile{
							.winding = 0,
							.hasIncrements = false,
							.increments = std::array<Increment, TILE_SIZE * TILE_SIZE>()
							});
					}
				}

				tileCount += count;
			}
		}

		uint32_t tileCount = 0;
		uint32_t visibleTileCount = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			PathRender& path = Globals::AllPaths.paths[pathIndex];
			uint32_t count = path.startTileIndex;
			path.startTileIndex = tileCount;
			tileCount += count;
			path.endTileIndex = tileCount - 1;

			uint32_t visibleCount = path.startVisibleTileIndex;
			path.startVisibleTileIndex = visibleTileCount;
			visibleTileCount += visibleCount;
		}

		std::vector<uint32_t> indices;
		indices.resize(Globals::AllPaths.paths.size());
		std::iota(indices.begin(), indices.end(), 0);

		{
			Timer timer;
			std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
			{
				PathRender& path = Globals::AllPaths.paths[pathIndex];
				if (!Flattening::IsInsideViewSpace(path.bbox.min) && !Flattening::IsInsideViewSpace(path.bbox.max))
				{
					return;
				}

				Rasterizer rast(path.bbox, pathIndex);
				rast.FillFromArray(pathIndex);
			});
			SR_INFO("Time for filling: {0}", timer.ElapsedMillis());
		}

		// 4.2 Calculate corrent count and indices for vertices of each path
		uint32_t accumCount = 0;
		uint32_t accumTileCount = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			PathRender& path = Globals::AllPaths.paths[pathIndex];
			if (!Flattening::IsInsideViewSpace(path.bbox.min) && !Flattening::IsInsideViewSpace(path.bbox.max))
			{
				continue;
			}

			Rasterizer rast(path.bbox, pathIndex);

			auto [coarseQuadCount, fineQuadCount] = rast.CalculateNumberOfQuads();
			path.startSpanQuadIndex = accumCount;
			path.startTileQuadIndex = accumCount + coarseQuadCount;
			accumCount += coarseQuadCount + fineQuadCount;

			path.startVisibleTileIndex = accumTileCount;
			accumTileCount += fineQuadCount;
		}

		m_TileBuilder.vertices.resize(accumCount * 4);
		const size_t numberOfIndices = accumCount * 6;
		m_TileBuilder.indices.reserve(numberOfIndices);
		uint32_t base = 0;
		for (size_t i = 0; i < numberOfIndices; i += 6)
		{
			m_TileBuilder.indices.push_back(base);
			m_TileBuilder.indices.push_back(base + 1);
			m_TileBuilder.indices.push_back(base + 2);
			m_TileBuilder.indices.push_back(base);
			m_TileBuilder.indices.push_back(base + 2);
			m_TileBuilder.indices.push_back(base + 3);
			base += 4;
		}

		// 4.3 The rest

		Timer timer;
		std::for_each(std::execution::par, indices.cbegin(), indices.cend(), [this](uint32_t pathIndex)
		{
			const PathRender& path = Globals::AllPaths.paths[pathIndex];
			if (!Flattening::IsInsideViewSpace(path.bbox.min) && !Flattening::IsInsideViewSpace(path.bbox.max))
			{
				return;
			}

			Rasterizer rast(path.bbox, pathIndex);
			rast.Coarse(m_TileBuilder);
			rast.Finish(m_TileBuilder);
		});
		SR_INFO("Time in ms: {0}", timer.ElapsedMillis());
#else
		uint32_t total = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			const PathRender& path = Globals::AllPaths.paths[pathIndex];
			if (!Flattening::IsInsideViewSpace(path.bbox.min) || !Flattening::IsInsideViewSpace(path.bbox.max))
			{
				continue;
			}

			Rasterizer rast(path.bbox);
			rast.FillFromArray(pathIndex);

			m_TileBuilder.color = path.color;
			rast.Finish(m_TileBuilder);

			if (++total % 10000 == 0)
			{
				SR_TRACE("Processed {0} paths", total);
			}
		}
#endif

		SR_INFO("Total time: {0} ms", globalTimer.ElapsedMillis());
	}

	void Application::Shutdown()
	{
		Renderer::Shutdown();
		m_Window->Close();
	}

	OrthographicCamera camera(0, 1280.0f, 0.0f, 720.0f, -100.0f, 100.0f);

	void Application::Run()
	{
		Ref<Shader> shader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "Main.vert", Filesystem::AssetsPath() / "shaders" / "Main.frag");

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_R8,
			ATLAS_SIZE,
			ATLAS_SIZE,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			m_TileBuilder.atlas.data()
		);

		GLuint vbo = 0;
		GLuint ibo = 0;
		GLuint vao = 0;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, m_TileBuilder.vertices.size() * sizeof(Vertex), m_TileBuilder.vertices.data(), GL_STREAM_DRAW);

		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			m_TileBuilder.indices.size() * sizeof(uint32_t),
			m_TileBuilder.indices.data(),
			GL_STREAM_DRAW
		);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(
			0,
			2,
			GL_INT,
			GL_FALSE,
			sizeof(Vertex),
			(const void*)0
			);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(
			1,
			2,
			GL_UNSIGNED_INT,
			GL_FALSE,
			sizeof(Vertex),
			(const void*)8
			);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(
			2,
			4,
			GL_UNSIGNED_BYTE,
			GL_TRUE,
			sizeof(Vertex),
			(const void*)16
			);

		shader->Bind();

		glUniform2ui(0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glUniform2ui(1, ATLAS_SIZE, ATLAS_SIZE);
		glBindTextureUnit(0, tex);

		camera.SetPosition(glm::vec3(0, 0, 0.5f));

		m_Running = true;
		while (m_Running)
		{
			Timer timer;

			glClearColor(1.0, 1.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawElements(
				GL_TRIANGLES,
				m_TileBuilder.indices.size(),
				GL_UNSIGNED_INT,
				nullptr
			);

			//glFinish();

			m_Window->OnUpdate();
		}
	}

	void Application::OnWindowClose()
	{
		m_Running = false;
	}

	void Application::OnKeyPressed(int key, int repeat)
	{
	}

	void Application::OnKeyReleased(int key)
	{
	}

	void Application::OnMousePressed(int key, int repeat)
	{
	}

	void Application::OnMouseReleased(int key)
	{
	}

	void Application::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

}
