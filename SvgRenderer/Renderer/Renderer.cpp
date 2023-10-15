#include "Renderer.h"

#include "Core/Filesystem.h"

#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/VertexArray.h"

#include <glad/glad.h>

namespace SvgRenderer {

	float Curve::calculateApproximatePerimeter() const
	{
		switch (type)
		{
		case CurveType::Bezier3:
		{
			const glm::vec2& p1 = this->as.bezier3.p1;
			const glm::vec2& p2 = this->as.bezier3.p2;
			const glm::vec2& p3 = this->as.bezier3.p3;
			float chordLength = glm::length(p3 - p0);
			float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1) + glm::length(p3 - p2);
			float approxLength = (chordLength + controlNetLength) / 2.0f;
			return approxLength;
		}
		break;
		case CurveType::Bezier2:
		{
			const glm::vec2& p1 = this->as.bezier2.p1;
			const glm::vec2& p2 = this->as.bezier2.p2;
			float chordLength = glm::length(p2 - p0);
			float controlNetLength = glm::length(p1 - p0) + glm::length(p2 - p1);
			float approxLength = (chordLength + controlNetLength) / 2.0f;
			return approxLength;
		}
		break;
		case CurveType::Line:
		{
			const glm::vec2& p1 = this->as.line.p1;
			return glm::length(p1 - p0);
		}
		break;
		case CurveType::None:
			break;
		}

		return 0.0f;
	}

	Curve Curve::split(float _t0, float _t1) const
	{
		Curve res{};
		res.type = type;

		switch (type)
		{
		case CurveType::Bezier3:
		{
			const glm::vec2& p1 = as.bezier3.p1;
			const glm::vec2& p2 = as.bezier3.p2;
			const glm::vec2& p3 = as.bezier3.p3;

			// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-b%C3%A9zier-curve-by-reusing-a-basic-b%C3%A9zier-curve-function
			float t0 = _t0;
			float t1 = _t1;
			float u0 = (1.0f - t0);
			float u1 = (1.0f - t1);

			glm::vec2 q0 = ((u0 * u0 * u0) * p0) +
				((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * p1) +
				((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * p2) +
				((t0 * t0 * t0) * p3);
			glm::vec2 q1 = ((u0 * u0 * u1) * p0) +
				((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * p1) +
				((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * p2) +
				((t0 * t0 * t1) * p3);
			glm::vec2 q2 = ((u0 * u1 * u1) * p0) +
				((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * p1) +
				((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * p2) +
				((t0 * t1 * t1) * p3);
			glm::vec2 q3 = ((u1 * u1 * u1) * p0) +
				((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * p1) +
				((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * p2) +
				((t1 * t1 * t1) * p3);

			res.p0 = q0;
			res.as.bezier3.p1 = q1;
			res.as.bezier3.p2 = q2;
			res.as.bezier3.p3 = q3;
		}
		break;
		case CurveType::Bezier2:
		{
			// Elevate the bezier2 to a bezier3
			res.type = CurveType::Bezier3;

			const glm::vec2& p1 = as.bezier2.p1;
			const glm::vec2& p2 = as.bezier2.p1;
			const glm::vec2& p3 = as.bezier2.p2;

			// Degree elevated quadratic bezier curve
			glm::vec2 pr0 = p0;
			glm::vec2 pr1 = (1.0f / 3.0f) * p0 + (2.0f / 3.0f) * p1;
			glm::vec2 pr2 = (2.0f / 3.0f) * p1 + (1.0f / 3.0f) * p2;
			glm::vec2 pr3 = p3;

			// Interpolate the curve
			// Taken from https://stackoverflow.com/questions/878862/drawing-part-of-a-b%C3%A9zier-curve-by-reusing-a-basic-b%C3%A9zier-curve-function
			float t0 = _t0;
			float t1 = _t1;
			float u0 = (1.0f - t0);
			float u1 = (1.0f - t1);

			glm::vec2 q0 = ((u0 * u0 * u0) * pr0) +
				((t0 * u0 * u0 + u0 * t0 * u0 + u0 * u0 * t0) * pr1) +
				((t0 * t0 * u0 + u0 * t0 * t0 + t0 * u0 * t0) * pr2) +
				((t0 * t0 * t0) * pr3);
			glm::vec2 q1 = ((u0 * u0 * u1) * pr0) +
				((t0 * u0 * u1 + u0 * t0 * u1 + u0 * u0 * t1) * pr1) +
				((t0 * t0 * u1 + u0 * t0 * t1 + t0 * u0 * t1) * pr2) +
				((t0 * t0 * t1) * pr3);
			glm::vec2 q2 = ((u0 * u1 * u1) * pr0) +
				((t0 * u1 * u1 + u0 * t1 * u1 + u0 * u1 * t1) * pr1) +
				((t0 * t1 * u1 + u0 * t1 * t1 + t0 * u1 * t1) * pr2) +
				((t0 * t1 * t1) * pr3);
			glm::vec2 q3 = ((u1 * u1 * u1) * pr0) +
				((t1 * u1 * u1 + u1 * t1 * u1 + u1 * u1 * t1) * pr1) +
				((t1 * t1 * u1 + u1 * t1 * t1 + t1 * u1 * t1) * pr2) +
				((t1 * t1 * t1) * pr3);

			res.p0 = q0;
			res.as.bezier3.p1 = q1;
			res.as.bezier3.p2 = q2;
			res.as.bezier3.p3 = q3;
		}
		break;
		case CurveType::Line:
		{
			glm::vec2 dir = this->as.line.p1 - this->p0;
			res.p0 = (_t0 * dir) + this->p0;
			res.as.line.p1 = (_t1 * dir) + this->p0;
		}
		break;
		case CurveType::None:
			break;
		}

		return res;
	}

	float Path::calculateApproximatePerimeter() const
	{
		float approxPerimeter = 0.0f;
		for (int curvei = 0; curvei < this->numCurves; curvei++)
		{
			const Curve& curve = this->curves[curvei];
			approxPerimeter += curve.calculateApproximatePerimeter();
		}

		return approxPerimeter;
	}

	Ref<Shader> Renderer::s_MainShader;
	Ref<Shader> Renderer::s_MainShader3D;
	Ref<Shader> Renderer::s_DisplayFboShader;

	const OrthographicCamera* Renderer::m_ActiveCamera = nullptr;
	std::vector<Vertex2D> Renderer::s_Vertices;
	std::vector<Vertex3D> Renderer::s_Vertices3D;
	RendererStats Renderer::s_Stats;

	void Renderer::Init(uint32_t initWidth, uint32_t initHeight)
	{
		std::filesystem::path shadersPath(Filesystem::AssetsPath() / "shaders");
		s_MainShader = Shader::Create(shadersPath / "main.vert", shadersPath / "main.frag");
		s_MainShader3D = Shader::Create(shadersPath / "main3D.vert", shadersPath / "main3D.frag");
		s_DisplayFboShader = Shader::Create(shadersPath / "DisplayFbo.vert", shadersPath / "DisplayFbo.frag");

		glViewport(0, 0, initWidth, initHeight);
	}

	void Renderer::Shutdown()
	{
		s_MainShader.reset();
	}

	void Renderer::BeginScene(const OrthographicCamera& camera)
	{
		m_ActiveCamera = &camera;
		s_Vertices.clear();
		s_Stats.drawCalls = 0;
	}

	void Renderer::EndScene()
	{
		{
			Ref<VertexArray> vao = VertexArray::Create();

			Ref<VertexBuffer> vbo = VertexBuffer::Create(s_Vertices.data(), static_cast<uint32_t>(s_Vertices.size() * sizeof(Vertex2D)));
			vbo->SetLayout(Vertex2D::GetBufferLayout());

			vao->AddVertexBuffer(vbo);

			vao->Bind();
			s_MainShader->Bind();

			SR_ASSERT(m_ActiveCamera != nullptr, "Camera was nullptr");
			s_MainShader->SetUniformMat4(0, m_ActiveCamera->GetViewProjectionMatrix());

			glDrawArrays(GL_TRIANGLES, 0, static_cast<uint32_t>(s_Vertices.size()));
			++s_Stats.drawCalls;
		}

		{
			Ref<VertexArray> vao = VertexArray::Create();

			Ref<VertexBuffer> vbo = VertexBuffer::Create(s_Vertices3D.data(), static_cast<uint32_t>(s_Vertices3D.size() * sizeof(Vertex3D)));
			vbo->SetLayout(Vertex3D::GetBufferLayout());

			vao->AddVertexBuffer(vbo);

			vao->Bind();
			s_MainShader->Bind();

			SR_ASSERT(m_ActiveCamera != nullptr, "Camera was nullptr");
			s_MainShader->SetUniformMat4(0, m_ActiveCamera->GetViewProjectionMatrix());

			glDrawArrays(GL_TRIANGLES, 0, static_cast<uint32_t>(s_Vertices3D.size()));
			++s_Stats.drawCalls;
		}
	}

	void Renderer::RenderFramebuffer(const Ref<Framebuffer>& fbo)
	{
		Ref<VertexArray> emptyVao = VertexArray::Create();

		//Framebuffer::BindDefaultFramebuffer();
		emptyVao->Bind();
		s_DisplayFboShader->Bind();
		glDisable(GL_DEPTH_TEST);
		glBindTextureUnit(0, fbo->GetColorAttachmentRendererID());
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	void Renderer::DrawTriangle(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2)
	{
		const glm::vec4 color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);

		s_Vertices.push_back({ p0, color });
		s_Vertices.push_back({ p1, color });
		s_Vertices.push_back({ p2, color });
	}

	void Renderer::DrawTriangle3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
	{
		const glm::vec4 color = glm::vec4(1.0f, 0.5f, 0.2f, 1.0f);

		s_Vertices3D.push_back({ p0, color });
		s_Vertices3D.push_back({ p1, color });
		s_Vertices3D.push_back({ p2, color });
	}

	void Renderer::DrawLine(const glm::vec2& start, const glm::vec2& end)
	{
		float strokeWidth = 10.0f;

		glm::vec2 direction = end - start;
		glm::vec2 normalDirection = glm::normalize(direction);
		glm::vec2 perpVector = glm::vec2(-normalDirection.y, normalDirection.x);

		glm::vec2 bottomLeft = start - (perpVector * strokeWidth * 0.5f);
		glm::vec2 bottomRight = start + (perpVector * strokeWidth * 0.5f);
		glm::vec2 topLeft = end + (perpVector * strokeWidth * 0.5f);
		glm::vec2 topRight = end - (perpVector * strokeWidth * 0.5f);

		DrawTriangle(bottomLeft, bottomRight, topLeft);
		DrawTriangle(bottomLeft, topLeft, topRight);
	}

	void Renderer::DrawQuad(const glm::vec2& start, const glm::vec2& size)
	{
		glm::vec2 min = start + (size * -0.5f);
		glm::vec2 max = start + (size * 0.5f);

		const uint32_t indices[] = {
			0, 1, 2,
			0, 2, 3
		};

		Ref<IndexBuffer> ibo = IndexBuffer::Create(indices, 6);

		const glm::vec4 color = glm::vec4(1, 1, 1, 1);

		std::vector<Vertex2D> vertices;

		Vertex2D vert;
		vert.color = color;

		vert.position = min;
		vertices.push_back(vert);

		vert.position = { min.x, max.y };
		vertices.push_back(vert);

		vert.position = max;
		vertices.push_back(vert);

		vert.position = { max.x, min.y };
		vertices.push_back(vert);

		Ref<VertexBuffer> vbo = VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(Vertex2D));
		vbo->SetLayout(Vertex2D::GetBufferLayout());

		Ref<VertexArray> vao = VertexArray::Create();
		vao->AddVertexBuffer(vbo);
		vao->SetIndexBuffer(ibo);

		vao->Bind();
		s_MainShader->Bind();

		SR_ASSERT(m_ActiveCamera != nullptr, "Camera was nullptr");
		s_MainShader->SetUniformMat4(0, m_ActiveCamera->GetViewProjectionMatrix());

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	Path2DContext* Renderer::BeginPath(const glm::vec2& start, const glm::mat4& transform)
	{
		Path2DContext* context = new Path2DContext();

		float strokeWidth = 10.0f;

		context->transform = transform;

		Path_Vertex2DLine vert = {};
		vert.position = start;
		vert.color = glm::vec4(1, 1, 1, 1);
		vert.thickness = strokeWidth;
		context->data.emplace_back(vert);

		return context;
	}

	static bool compare(float x, float y, float epsilon = std::numeric_limits<float>::min())
	{
		return std::abs(x - y) <= epsilon * std::max(1.0f, std::max(std::abs(x), std::abs(y)));
	}

	static bool compare(const glm::vec2& vec1, const glm::vec2& vec2, float epsilon = std::numeric_limits<float>::min())
	{
		return compare(vec1.x, vec2.x, epsilon) && compare(vec1.y, vec2.y, epsilon);
	}

	static glm::vec3 transformVertVec3(const glm::vec2& vert, const glm::mat4& transform)
	{
		glm::vec4 translated = glm::vec4(vert.x, vert.y, 0.0f, 1.0f);
		translated = transform * translated;
		// NOTE: Add a small offset to the z-coord to avoid z-fighting
		return { translated.x, translated.y, translated.z + 0.0001f };
	}

	bool Renderer::EndPath(Path2DContext* path, bool closePath)
	{
		SR_ASSERT(path != nullptr, "Null path.");

		if (path->data.size() <= 1)
		{
			SR_WARN("Corrupted path data found");
			return false;
		}

		// NOTE: Do two loops:
		//
		//       The first loop extrudes all the vertices
		//       and forms the tesselated path. It also creates
		//       any bevels/miters/rounded corners and adds
		//       them immediately and just saves the connection
		//       points for the second loop.
		//
		//       The second loop
		//       connects the verts into quads to form the stroke

		// TODO: Clean this up. Path's should never have a duplicate start/end point in the first
		//       place if it's a closed path, that should be implicit. Instead we should normalize paths
		//       and make sure that if a path gets closed the endpoint != the start point.
		//       Here's a Github issue to track this:
		//         https://github.com/ambrosiogabe/MathAnimation/issues/104
		//       ID for code search: %BW7n4C2kfxQtpij6tHL
		bool firstPointIsSameAsLastPoint = path->data.size() > 0
			? path->data[0].position == path->data[path->data.size() - 1].position
			: true;

		int endPoint = firstPointIsSameAsLastPoint
			? (int)path->data.size() - 1
			: (int)path->data.size();
		for (int vertIndex = 0; vertIndex < (int)path->data.size(); vertIndex++)
		{
			Path_Vertex2DLine& vertex = path->data[vertIndex];
			glm::vec2 currentPos = vertex.position;
			glm::vec2 nextPos = vertIndex + 1 < endPoint
				? path->data[vertIndex + 1].position
				: closePath
				? path->data[(vertIndex + 1) % endPoint].position
				: path->data[endPoint - 1].position;
			glm::vec2 previousPos = vertIndex > 0
				? path->data[vertIndex - 1].position
				: closePath
				? path->data[endPoint - 1].position
				: path->data[0].position;
			const glm::vec4& currentColor = vertex.color;

			glm::vec2 dirA = glm::normalize(currentPos - previousPos);
			glm::vec2 dirB = glm::normalize(nextPos - currentPos);
			if (compare(dirA, -1.0f * dirB))
			{
				// Vectors are pointing away from each other. This means finding a bisection vector would be
				// inconclusive since it could point either way. So we'll just wiggle the vector a bit to
				// make the math not degrade to inf's and nan's
				dirA.x += 0.0000001f;
			}
			glm::vec2 bisectionPerp = glm::normalize(dirA + dirB);
			glm::vec2 secondLinePerp = glm::vec2{ -dirB.y, dirB.x };
			// This is the miter
			glm::vec2 bisection = glm::vec2{ -bisectionPerp.y, bisectionPerp.x };
			glm::vec2 extrusionNormal = bisection;
			float bisectionDotProduct = glm::dot(bisection, secondLinePerp);
			float miterThickness = vertex.thickness / bisectionDotProduct;
			if (compare(bisectionDotProduct, 0.0f, 0.01f))
			{
				// Clamp the miter if the joining curves are almost parallell
				miterThickness = vertex.thickness;
			}

			constexpr float strokeMiterLimit = 2.0f;
			bool shouldConvertToBevel = glm::abs(miterThickness / vertex.thickness) > strokeMiterLimit;
			if (shouldConvertToBevel)
			{
				float firstBevelWidth = vertex.thickness / glm::dot(bisection, dirA) * 0.5f;
				float secondBevelWidth = vertex.thickness / glm::dot(bisection, dirB) * 0.5f;
				glm::vec2 firstPoint = currentPos + (bisectionPerp * firstBevelWidth);
				glm::vec2 secondPoint = currentPos + (bisectionPerp * secondBevelWidth);

				float centerBevelWidth = vertex.thickness / glm::dot(bisectionPerp, glm::normalize(currentPos - previousPos));
				centerBevelWidth = glm::min(centerBevelWidth, vertex.thickness);
				glm::vec2 centerPoint = currentPos + (bisection * centerBevelWidth);
				Renderer::DrawTriangle(firstPoint, secondPoint, centerPoint);

				// Save the "front" and "back" for the connection loop
				vertex.frontP1 = centerPoint;
				vertex.frontP2 = secondPoint;

				vertex.backP1 = centerPoint;
				vertex.backP2 = firstPoint;

				// -----------
				continue; // NOTE: The continue here. This is basically like an exit early thing
				// -----------
			}

			// If we're drawing the beginning/end of the path, just
			// do a straight cap on the line segment
			if (vertIndex == 0 && !closePath)
			{
				glm::vec2 normal = glm::normalize(nextPos - currentPos);
				extrusionNormal = glm::vec2{ -normal.y, normal.x };
				miterThickness = vertex.thickness;
			}
			else if (vertIndex == endPoint - 1 && !closePath)
			{
				glm::vec2 normal = glm::normalize(currentPos - previousPos);
				extrusionNormal = glm::vec2{ -normal.y, normal.x };
				miterThickness = vertex.thickness;
			}

			// If we're doing a bevel or something, that uses special vertices
			// to join segments. Otherwise we can just use the extrusion normal
			// and miterThickness to join the segments
			vertex.frontP1 = vertex.position + extrusionNormal * miterThickness * 0.5f;
			vertex.frontP2 = vertex.position - extrusionNormal * miterThickness * 0.5f;

			vertex.backP1 = vertex.position + extrusionNormal * miterThickness * 0.5f;
			vertex.backP2 = vertex.position - extrusionNormal * miterThickness * 0.5f;
		}

		// NOTE: This is some weird shenanigans in order to get the path
		//       to close correctly and join the last vertex to the first vertex
		if (!closePath)
		{
			endPoint--;
		}

		// TODO: Stroke width scales with the object and it probably shouldn't
		//glm::vec3 scale, translation, skew;
		//glm::quat orientation;
		//glm::vec4 perspective;
		//glm::decompose(path->transform, scale, orientation, translation, skew, perspective);
		//glm::vec3 eulerAngles = glm::eulerAngles(orientation);
		//glm::mat4 unscaledMatrix = CMath::calculateTransform(
		//	Vec3{eulerAngles.x, eulerAngles.y, eulerAngles.z},
		//	Vec3{ 1, 1, 1 },
		//	Vec3{translation.x, translation.y, translation.z}
		//);

		for (int vertIndex = 0; vertIndex < endPoint; vertIndex++)
		{
			const Path_Vertex2DLine& vertex = path->data[vertIndex % path->data.size()];
			const glm::vec4& color = vertex.color;
			const Path_Vertex2DLine& nextVertex = path->data[(vertIndex + 1) % path->data.size()];
			const glm::vec4& nextColor = nextVertex.color;

			DrawTriangle3D(
				transformVertVec3(vertex.frontP1, path->transform),
				transformVertVec3(vertex.frontP2, path->transform),
				transformVertVec3(nextVertex.backP1, path->transform));
			DrawTriangle3D(
				transformVertVec3(vertex.frontP2, path->transform),
				transformVertVec3(nextVertex.backP2, path->transform),
				transformVertVec3(nextVertex.backP1, path->transform));
		}

		return true;
	}

	void Renderer::LineTo(Path2DContext* path, const glm::vec2& point)
	{
		SR_ASSERT(path != nullptr, "Null path.");

		float strokeWidth = 10.0f;

		Path_Vertex2DLine vert;
		vert.position = point;
		vert.color = glm::vec4(1, 1, 1, 1);
		vert.thickness = strokeWidth;

		LineToInternal(path, vert, true);
	}

	static float lengthSquared(const glm::vec2& v)
	{
		float len = glm::length(v);
		return len * len;
	}

	static glm::vec2 bezier2(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, float t)
	{
		return (1.0f - t) * ((1.0f - t) * p0 + t * p1) + t * ((1.0f - t) * p1 + t * p2);
	}

	void Renderer::QuadTo(Path2DContext* path, const glm::vec2& p1, const glm::vec2& p2)
	{
		SR_ASSERT(path != nullptr, "Null path.");
		SR_ASSERT(path->data.size() > 0, "Cannot do a quadTo on an empty path.");

		glm::vec2 p0 = path->data[path->data.size() - 1].position;

		{
			// Add raw curve data
			Curve rawCurve{};
			rawCurve.type = CurveType::Bezier2;
			rawCurve.p0 = p0;
			rawCurve.as.bezier2.p1 = p1;
			rawCurve.as.bezier2.p2 = p2;

			path->approximateLength += rawCurve.calculateApproximatePerimeter();
			path->rawCurves.emplace_back(rawCurve);
		}

		// Estimate the length of the bezier curve to get an approximate for the
		// number of line segments to use
		glm::vec2 chord1 = p1 - p0;
		glm::vec2 chord2 = p2 - p1;
		float chordLengthSq = lengthSquared(chord1) + lengthSquared(chord2);
		float lineLengthSq = lengthSquared(p2 - p0);
		float approxLength = glm::sqrt(lineLengthSq + chordLengthSq) / 2.0f;
		int numSegments = (int)(approxLength * 40.0f);
		for (int i = 1; i < numSegments - 1; i++)
		{
			float t = (float)i / (float)numSegments;
			glm::vec2 interpPoint = bezier2(p0, p1, p2, t);
			LineToInternal(path, interpPoint, false);
		}

		LineToInternal(path, p2, false);
	}

	void Renderer::LineToInternal(Path2DContext* path, const glm::vec2& point, bool addRawCurve)
	{
		SR_ASSERT(path != nullptr, "Null path.");
		if (path->data.size() <= 0)
		{
			return;
		}

		float strokeWidth = 10.0f;

		Path_Vertex2DLine vert;
		vert.position = point;
		vert.color = glm::vec4(1, 1, 1, 1);
		vert.thickness = strokeWidth;

		// Don't add the vertex if it's going to itself
		glm::vec2 deltaToCursor = path->data[path->data.size() - 1].position - vert.position;
		if (!compare(deltaToCursor, { 0.0f, 0.0f }, 0.00001f))
		{
			if (addRawCurve)
			{
				Curve rawCurve;
				rawCurve.type = CurveType::Line;
				rawCurve.p0 = path->data[path->data.size() - 1].position;
				rawCurve.as.line.p1 = vert.position;

				path->approximateLength += rawCurve.calculateApproximatePerimeter();
				path->rawCurves.emplace_back(rawCurve);
			}

			path->data.emplace_back(vert);
		}
	}

	void Renderer::LineToInternal(Path2DContext* path, const Path_Vertex2DLine& vert, bool addRawCurve)
	{
		SR_ASSERT(path != nullptr, "Null path.");
		if (path->data.size() <= 0)
		{
			return;
		}

		// Don't add the vertex if it's going to itself
		glm::vec2 deltaToCursor = path->data[path->data.size() - 1].position - vert.position;
		if (!compare(deltaToCursor, { 0.0f, 0.0f }, 0.00001f))
		{
			if (addRawCurve)
			{
				Curve rawCurve;
				rawCurve.type = CurveType::Line;
				rawCurve.p0 = path->data[path->data.size() - 1].position;
				rawCurve.as.line.p1 = vert.position;

				path->approximateLength += rawCurve.calculateApproximatePerimeter();
				path->rawCurves.emplace_back(rawCurve);
			}

			path->data.emplace_back(vert);
		}
	}

}
