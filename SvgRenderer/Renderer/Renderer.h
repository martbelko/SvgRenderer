#pragma once

#include "Renderer/VertexBuffer.h"
#include "Renderer/Shader.h"
#include "Renderer/Framebuffer.h"

#include "Scene/OrthographicCamera.h"

#include <glm/glm.hpp>

namespace SvgRenderer {

	struct Vertex2D
	{
		glm::vec2 position;
		glm::vec4 color;

		static BufferLayout GetBufferLayout()
		{
			return BufferLayout({
				{ ShaderDataType::Float2 },
				{ ShaderDataType::Float4 }
			});
		}
	};

	struct Vertex3D
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec4 color;

		static BufferLayout GetBufferLayout()
		{
			return BufferLayout({
				{ ShaderDataType::Float3 },
				{ ShaderDataType::Float3 },
				{ ShaderDataType::Float4 }
				});
		}
	};

	enum class CurveType : uint8_t
	{
		None = 0,
		Line,
		Bezier2,
		Bezier3
	};

	struct Line
	{
		glm::vec2 p1;
	};

	struct Bezier2
	{
		glm::vec2 p1;
		glm::vec2 p2;
	};

	struct Bezier3
	{
		glm::vec2 p1;
		glm::vec2 p2;
		glm::vec2 p3;
	};

	struct Curve
	{
		CurveType type;
		// Every curve has at least one point
		glm::vec2 p0;
		union
		{
			Line line;
			Bezier2 bezier2;
			Bezier3 bezier3;
		} as;

		float calculateApproximatePerimeter() const;
		Curve split(float t0, float t1) const;
	};

	struct Path
	{
		Curve* curves;
		int numCurves;
		int maxCapacity;
		bool isHole;

		float calculateApproximatePerimeter() const;
	};

	struct Path_Vertex2DLine
	{
		glm::vec2 position;
		glm::vec4 color;
		float thickness;

		glm::vec2 frontP1, frontP2;
		glm::vec2 backP1, backP2;
	};


	struct Path2DContext
	{
		std::vector<Curve> rawCurves;
		std::vector<Path_Vertex2DLine> data;
		glm::mat4 transform;
		float approximateLength;
	};

	struct RendererStats
	{
		uint32_t drawCalls;
	};

	class Renderer
	{
	public:
		static void Init(uint32_t initWidth, uint32_t initHeight);
		static void Shutdown();

		static void BeginScene(const OrthographicCamera& camera);
		static void EndScene();

		static void RenderFramebuffer(const Ref<Framebuffer>& fbo);

		static void DrawTriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2);
		static void DrawTriangle3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);

		static void DrawLine(const glm::vec2& from, const glm::vec2& to);
		static void DrawQuad(const glm::vec2& start, const glm::vec2& size);

		static Path2DContext* BeginPath(const glm::vec2& start, const glm::mat4& transform);
		static bool EndPath(Path2DContext* path, bool closePath = true);

		static void LineTo(Path2DContext* path, const glm::vec2& point);
		static void QuadTo(Path2DContext* path, const glm::vec2& p1, const glm::vec2& p2);

		static const OrthographicCamera* GetCurrentCamera3D() { return m_ActiveCamera; }
	private:
		static void LineToInternal(Path2DContext* path, const glm::vec2& point, bool addRawCurve);
		static void LineToInternal(Path2DContext* path, const Path_Vertex2DLine& vert, bool addRawCurve);
	private:
		static Ref<Shader> s_MainShader;
		static Ref<Shader> s_MainShader3D;
		static Ref<Shader> s_DisplayFboShader;

		static const OrthographicCamera* m_ActiveCamera;

		static std::vector<Vertex2D> s_Vertices;
		static std::vector<Vertex3D> s_Vertices3D;

		static RendererStats s_Stats;
	};

}
