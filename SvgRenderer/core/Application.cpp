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

#include "Scene/OrthographicCamera.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace SvgRenderer {

	constexpr uint32_t SCREEN_WIDTH = 1900;
	constexpr uint32_t SCREEN_HEIGHT = 1000;

	uint32_t g_PathIndex = 0;

	Application Application::s_Instance;

	struct PathRender
	{
		uint32_t index;
		glm::vec4 color;
		glm::mat4 transform;
	};

	struct PathRenderCmd
	{
		PathCmd cmd;
		uint32_t pathIndex;
	};

	struct PathsContainer
	{
		std::vector<PathRender> paths;
		std::vector<PathRenderCmd> commands;
	};

	PathsContainer g_AllPaths;
	PathsContainer g_AllCommands;

	static void Render(const SvgNode* node, TileBuilder& builder)
	{
		switch (node->type)
		{
		case SvgNodeType::Path:
		{
			const SvgPath& path = node->as.path;

			std::vector<PathCmd> cmds;
			for (const SvgPath::Segment& seg : path.segments)
			{
				switch (seg.type)
				{
				case SvgPath::Segment::Type::MoveTo:
					cmds.push_back(PathCmd(MoveToCmd{ .point = seg.as.moveTo.p }));
					break;
				case SvgPath::Segment::Type::LineTo:
					cmds.push_back(PathCmd(LineToCmd{ .p1 = seg.as.lineTo.p }));
					break;
				case SvgPath::Segment::Type::Close:
					cmds.push_back(CloseCmd{});
					break;
				case SvgPath::Segment::Type::QuadTo:
					cmds.push_back(PathCmd(QuadToCmd{ .p1 = seg.as.quadTo.p1, .p2 = seg.as.quadTo.p2 }));
					break;
				case SvgPath::Segment::Type::CubicTo:
					cmds.push_back(PathCmd(CubicToCmd{ .p1 = seg.as.cubicTo.p1, .p2 = seg.as.cubicTo.p2, .p3 = seg.as.cubicTo.p3 }));
					break;
				// TODO: Implement others
				}
			}

			const SvgColor& c = path.fill.color;
			builder.color = { c.r, c.g, c.b, static_cast<uint8_t>(path.fill.opacity * 255.0f) };

			glm::vec4 normColor = { c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, path.fill.opacity };

			g_AllPaths.paths.push_back(PathRender{
				.index = g_PathIndex,
				.color = normColor,
				.transform = path.transform
			});

			for (const PathCmd& cmd : cmds)
			{
				g_AllPaths.commands.push_back(PathRenderCmd{
					.cmd = cmd,
					.pathIndex = g_PathIndex
				});
			}

			Rasterizer rast(SCREEN_WIDTH, SCREEN_HEIGHT);
			rast.Fill(cmds, path.transform);
			rast.Finish(builder);

			++g_PathIndex;
			break;
		}
		}

		for (const SvgNode* child : node->children)
			Render(child, builder);
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
		SvgNode* root = SvgParser::Parse("C:/Users/Martin/Desktop/test.svg");
		SR_TRACE("Parsing finish");

		Render(root, m_TileBuilder);
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
