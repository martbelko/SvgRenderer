#include "FillRenderer.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"

#include "Scene/OrthographicCamera.h"

#include <glad/glad.h>

namespace SvgRenderer {

	void FillRenderer::Init()
	{
		m_TriShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "tri.vert", Filesystem::AssetsPath() / "shaders" / "tri.frag");
		m_BezShader = Shader::Create(Filesystem::AssetsPath() / "shaders" / "bez.vert", Filesystem::AssetsPath() / "shaders" / "bez.frag");
	}

	void FillRenderer::BeginScene(const OrthographicCamera& camera)
	{
		m_Camera = &camera;
	}

	void FillRenderer::EndScene()
	{
		for (size_t i = 0; i < m_Paths.size(); ++i)
		{
			DrawPathInternal(m_Paths[i], i);
		}
	}

	void FillRenderer::DrawPath(const FillPath& path)
	{
		m_Paths.push_back(path);
	}

	void FillRenderer::DrawPathInternal(const FillPath& path, float depth)
	{
		glm::vec2 centroid = { 0, 0 }; // TODO: Maybe compute something better in the future ???
		glm::vec2 currentPoint = { 0, 0 };

		std::vector<glm::vec3> triVertices;
		std::vector<glm::vec3> bezVertices;
		for (const FillCommand& cmd : path.commands)
		{
			if (cmd.type == FillCurveType::Move)
			{
				currentPoint = cmd.as.move.pos;
			}
			else if (cmd.type == FillCurveType::Line)
			{
				glm::vec2 p0 = centroid;
				glm::vec2 p1 = currentPoint;
				glm::vec2 p2 = cmd.as.line.p1;

				triVertices.push_back({ p0.x, p0.y, depth });
				triVertices.push_back({ p1.x, p1.y, depth });
				triVertices.push_back({ p2.x, p2.y, depth });

				currentPoint = cmd.as.line.p1;
			}
			else if (cmd.type == FillCurveType::Bezier2)
			{
				glm::vec2 p0 = centroid;
				glm::vec2 p1 = currentPoint;
				glm::vec2 p2 = cmd.as.bezier2.p2;

				triVertices.push_back({ p0.x, p0.y, depth });
				triVertices.push_back({ p1.x, p1.y, depth });
				triVertices.push_back({ p2.x, p2.y, depth });

				bezVertices.push_back({ currentPoint.x, currentPoint.y, depth });
				bezVertices.push_back({ cmd.as.bezier2.p1.x, cmd.as.bezier2.p1.y, depth });
				bezVertices.push_back({ cmd.as.bezier2.p2.x, cmd.as.bezier2.p2.y, depth });

				currentPoint = cmd.as.bezier2.p2;
			}
			else
			{
				SR_ASSERT(false, "Not implemented");
			}
		}

		{
			Ref<VertexBuffer> triVbo = VertexBuffer::Create(triVertices.data(), triVertices.size() * sizeof(glm::vec3));
			triVbo->SetLayout({
				{ ShaderDataType::Float3 }
			});

			Ref<VertexArray> triVao = VertexArray::Create();
			triVao->AddVertexBuffer(triVbo);

			triVao->Bind();
			m_TriShader->Bind();
			m_TriShader->SetUniformMat4(0, m_Camera->GetViewProjectionMatrix());

			glDrawArrays(GL_TRIANGLES, 0, triVertices.size());
		}

		Ref<VertexBuffer> bezVbo = VertexBuffer::Create(bezVertices.data(), bezVertices.size() * sizeof(glm::vec3));
		bezVbo->SetLayout({
			{ ShaderDataType::Float3 }
		});

		Ref<VertexArray> bezVao = VertexArray::Create();
		bezVao->AddVertexBuffer(bezVbo);

		bezVao->Bind();
		m_BezShader->Bind();
		m_BezShader->SetUniformMat4(0, m_Camera->GetViewProjectionMatrix());

		glDrawArrays(GL_TRIANGLES, 0, bezVertices.size());
	}

	FillPathBuilder& FillPathBuilder::MoveTo(const glm::vec2& pos)
	{
		FillCommand cmd = FillCommand();
		cmd.type = FillCurveType::Move;
		cmd.as.move = FillCommandMove{ .pos = pos };
		m_Path.commands.push_back(std::move(cmd));

		return *this;
	}

	FillPathBuilder& FillPathBuilder::LineTo(const glm::vec2& p1)
	{
		FillCommand cmd = FillCommand();
		cmd.type = FillCurveType::Line;
		cmd.as.line = FillCommandLine{ .p1 = p1 };
		m_Path.commands.push_back(std::move(cmd));

		return *this;
	}

	FillPathBuilder& FillPathBuilder::QuadTo(const glm::vec2& p1, const glm::vec2& p2)
	{
		FillCommand cmd = FillCommand();
		cmd.type = FillCurveType::Bezier2;
		cmd.as.bezier2 = FillCommandBezier2{ .p1 = p1, .p2 = p2 };
		m_Path.commands.push_back(std::move(cmd));

		return *this;
	}

	FillPathBuilder& FillPathBuilder::Close()
	{
		FillCommand cmd = FillCommand();
		cmd.type = FillCurveType::Close;
		cmd.as.close = FillCommandClose{};
		m_Path.commands.push_back(std::move(cmd));

		return *this;
	}

}
