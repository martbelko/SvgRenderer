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

#include "Renderer/Pipeline/GPUPipeline.h"
#include "Renderer/Pipeline/CPUPipeline.h"

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

	static void AddFillPath(const SvgPath& path)
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

	static void AddStrokePath(const SvgPath& path)
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

	static void ProcessSvgNode(const SvgNode* node)
	{
		switch (node->type)
		{
		case SvgNodeType::Path:
		{
			const SvgPath& path = node->as.path;
			AddFillPath(path);
			if (path.stroke.hasStroke)
			{
				//AddStrokePath(path, builder);
			}

			break;
		}
		}

		for (const SvgNode* child : node->children)
		{
			ProcessSvgNode(child);
		}
	}

	static glm::vec2 ApplyTransform(const glm::mat4& transform, const glm::vec2& point)
	{
		return Globals::GlobalTransform * transform * glm::vec4(point, 1.0f, 1.0f);
	}

	static void TransformCurve(PathRenderCmd* cmd)
	{
		uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd->pathIndexCmdType);
		cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
		cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
		cmd->transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[2]);
	}

	static glm::vec2 GetPreviousPoint(const PathRender& path, uint32_t index)
	{
		if (index == path.startCmdIndex)
		{
			return glm::vec2(0, 0);
		}

		PathRenderCmd& prevCmd = Globals::AllPaths.commands[index - 1];
		uint32_t pathType = GET_CMD_TYPE(prevCmd.pathIndexCmdType);
		switch (pathType)
		{
		case MOVE_TO:
		case LINE_TO:
			return prevCmd.transformedPoints[0];
		case QUAD_TO:
			return prevCmd.transformedPoints[1];
		case CUBIC_TO:
			return prevCmd.transformedPoints[2];
		}
	}

	void Application::Init()
	{
		m_Window = Window::Create({
			.width = Globals::WindowWidth,
			.height = Globals::WindowHeight,
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

		Renderer::Init(Globals::WindowWidth, Globals::WindowHeight);

		Timer timerParse;
		SvgNode* root = SvgParser::Parse("C:/Users/user/Desktop/svgs/tiger.svg");
		SR_TRACE("Parsing: {0} ms", timerParse.ElapsedMillis());

		// This actually fills information about colors and other attributes from the SVG root node
		ProcessSvgNode(root);
		delete root;

		m_Pipeline = new GPUPipeline();
		SR_INFO("Running in mode\n");
		m_Pipeline->Init();
	}

	void Application::Shutdown()
	{
		Renderer::Shutdown();
		m_Window->Close();
	}

	void Application::HandleInput()
	{
		static glm::vec3 pos = { 0, 0, 0 };
		static float scale = 1.0f;

		float dist = 5.0f;

		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_A) == GLFW_PRESS)
		{
			pos.x += dist;
		}
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_D) == GLFW_PRESS)
		{
			pos.x -= dist;
		}
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS)
		{
			pos.y += dist;
		}
		if (glfwGetKey(m_Window->GetNativeWindow(), GLFW_KEY_S) == GLFW_PRESS)
		{
			pos.y -= dist;
		}

		Globals::GlobalTransform = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));
	}

	void Application::Run()
	{
		m_Running = true;
		while (m_Running)
		{
			Timer timer;

			//HandleInput();

			m_Pipeline->Render();
			m_Pipeline->Final();
			glFinish();

			m_Window->OnUpdate();

			for (const Event& e : m_Window->GetAllEvents())
			{
				switch (e.type)
				{
				case EventType::Close:
					OnWindowClose();
					break;
				case EventType::Resize:
					OnViewportResize(e.as.windowResizeEvent.width, e.as.windowResizeEvent.height);
					break;
				}
			}
			m_Window->ClearEvents();

			SR_INFO("Frametime: {0} ms", timer.ElapsedMillis());
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
		Globals::WindowWidth = width;
		Globals::WindowHeight = height;
		glViewport(0, 0, width, height);
	}

}
