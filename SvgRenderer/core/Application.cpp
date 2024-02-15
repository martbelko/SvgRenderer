#include "Application.h"

#include "Core/Filesystem.h"
#include "Core/SvgParser.h"

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

#include "Scene/OrthographicCamera.h"

#include "Utils/BoundingBox.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <future>

namespace SvgRenderer {

	constexpr uint32_t SCREEN_WIDTH = 1900;
	constexpr uint32_t SCREEN_HEIGHT = 1000;

	uint32_t g_PathIndex = 0;

	Application Application::s_Instance;

	static void Render(const SvgNode* node, TileBuilder& builder)
	{
		switch (node->type)
		{
		case SvgNodeType::Path:
		{
			const SvgPath& path = node->as.path;

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
					cmds.push_back(PathCmd(LineToCmd{ .p1 = seg.as.lineTo.p }));
					break;
					{
						if (seg.as.lineTo.p != first)
						{
							cmds.push_back(PathCmd(LineToCmd{ .p1 = seg.as.lineTo.p }));
						}

						last = seg.as.lineTo.p;
						break;
					}
				case SvgPath::Segment::Type::Close:
					cmds.push_back(CloseCmd{});
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

			const SvgColor& c = path.fill.color;
			builder.color = { c.r, c.g, c.b, static_cast<uint8_t>(path.fill.opacity * 255.0f) };

			Globals::AllPaths.paths.push_back(PathRender{
				.startCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size()),
				.endCmdIndex = static_cast<uint32_t>(Globals::AllPaths.commands.size() + cmds.size() - 1),
				.transform = path.transform,
				.color = builder.color,
				.bbox = BoundingBox()
			});

			for (const PathCmd& cmd : cmds)
			{
				uint32_t index = Globals::AllPaths.paths.size() - 1; // -1, since we added path in the previous lines
				uint32_t pathIndexCmdType = MAKE_CMD_PATH_INDEX(0, index);
				pathIndexCmdType = MAKE_CMD_TYPE(pathIndexCmdType, static_cast<uint32_t>(cmd.type));

				std::array<glm::vec2, 3> points;
				points[0] = cmd.as.cubicTo.p1;
				points[1] = cmd.as.cubicTo.p2;
				points[2] = cmd.as.cubicTo.p3;

				Globals::AllPaths.commands.push_back(PathRenderCmd{
					.pathIndexCmdType = pathIndexCmdType,
					.points = points
				});
			}

			//Rasterizer rast(SCREEN_WIDTH, SCREEN_HEIGHT);
			//rast.Fill(cmds, path.transform);
			//rast.Finish(builder);

			++g_PathIndex;
			break;
		}
		}

		for (const SvgNode* child : node->children)
		{
			Render(child, builder);
		}
	}

	static glm::vec2 ApplyTransform(const glm::mat3& transform, const glm::vec2& point)
	{
		return Globals::GlobalTransform * glm::vec4(transform * glm::vec3(point, 1.0f), 1.0f);
	}

	static void TransformPath(uint32_t pathIndex)
	{
		PathRender& path = Globals::AllPaths.paths[pathIndex];
		for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
		{
			PathRenderCmd& cmd = Globals::AllPaths.commands[i];
			uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd.pathIndexCmdType);
			uint32_t cmdType = GET_CMD_TYPE(cmd.pathIndexCmdType);
			switch (cmdType)
			{
			case MOVE_TO:
			case LINE_TO:
				cmd.transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[0]);
				break;
			case QUAD_TO:
				cmd.transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[0]);
				cmd.transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[1]);
				break;
			case CUBIC_TO:
				cmd.transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[0]);
				cmd.transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[1]);
				cmd.transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd.points[2]);
				break;
			}
		}
	}

	static void TransformPathAsync(uint32_t pathIndex)
	{
		PathRender& path = Globals::AllPaths.paths[pathIndex];

		auto transformCurve = [](PathRenderCmd* cmd)
		{
			uint32_t pathIndex = GET_CMD_PATH_INDEX(cmd->pathIndexCmdType);
			uint32_t cmdType = GET_CMD_TYPE(cmd->pathIndexCmdType);
			switch (cmdType)
			{
			case MOVE_TO:
			case LINE_TO:
				cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
				break;
			case QUAD_TO:
				cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
				cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
				break;
			case CUBIC_TO:
				cmd->transformedPoints[0] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[0]);
				cmd->transformedPoints[1] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[1]);
				cmd->transformedPoints[2] = ApplyTransform(Globals::AllPaths.paths[pathIndex].transform, cmd->points[2]);
				break;
			}
		};

		std::vector<std::future<void>> futures;
		futures.reserve(path.endCmdIndex - path.startCmdIndex + 1);

		for (uint32_t i = path.startCmdIndex; i <= path.endCmdIndex; i++)
		{
			futures.push_back(std::async(std::launch::async, transformCurve, &Globals::AllPaths.commands[i]));
		}

		for (auto& f : futures)
		{
			f.wait();
		}
	}

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

		Renderer::Init(initWidth, initHeight);

		SR_TRACE("Parsing start");
		SvgNode* root = SvgParser::Parse("C:/Users/Martin/Desktop/svgs/paris.svg");
		SR_TRACE("Parsing finish");

		// This actually fills information about colors and other attributes from the SVG root node
		Render(root, m_TileBuilder);

		// Everything after this line may be done in each frame

#define ASYNC 0
		// 1.step: Transform the paths
#if ASYNC == 1
		std::vector<std::future<void>> futures;
		futures.reserve(Globals::AllPaths.paths.size());

		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); ++pathIndex)
		{
			futures.push_back(std::async(std::launch::async, TransformPathAsync, pathIndex));
		}

		for (auto& f : futures)
		{
			f.wait();
		}
#else
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); ++pathIndex)
		{
			TransformPath(pathIndex);
		}
#endif
		// 2.step: Calculate number of simple commands and their indexes for each path and each command in the path
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

		// 3.step: Simplify the commands and store in the array
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

		// 4.step: The rest
		uint32_t total = 0;
		for (uint32_t pathIndex = 0; pathIndex < Globals::AllPaths.paths.size(); pathIndex++)
		{
			const PathRender& path = Globals::AllPaths.paths[pathIndex];

			Rasterizer rast(path.bbox);
			rast.FillFromArray(pathIndex);

			m_TileBuilder.color = path.color;
			rast.Finish(m_TileBuilder);

			if (++total % 10000 == 0)
			{
				SR_TRACE("Processed {0} paths", total);
			}

			if (total == 46834)
			{
				//total += 10;
				//pathIndex += 10;
			}
		}
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
			glClearColor(1.0, 1.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawElements(
				GL_TRIANGLES,
				m_TileBuilder.indices.size(),
				GL_UNSIGNED_INT,
				nullptr
				);

			glFinish();

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
