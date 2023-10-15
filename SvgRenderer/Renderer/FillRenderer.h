#pragma once

#include <glm/glm.hpp>

namespace SvgRenderer {

	class OrthographicCamera;
	class Shader;

	enum class FillCurveType
	{
		Move = 0,
		Line,
		Bezier2,
		Close
	};

	struct FillCommandClose {};

	struct FillCommandMove
	{
		glm::vec2 pos;
	};

	struct FillCommandLine
	{
		glm::vec2 p1;
	};

	struct FillCommandBezier2
	{
		glm::vec2 p1;
		glm::vec2 p2;
	};

	struct FillCommand
	{
		FillCurveType type;
		union
		{
			FillCommandClose close;
			FillCommandMove move;
			FillCommandLine line;
			FillCommandBezier2 bezier2;
		} as;
	};

	class FillPath
	{
	public:
		std::vector<FillCommand> commands;
	};

	class FillPathBuilder
	{
	public:
		FillPathBuilder& MoveTo(const glm::vec2& pos);
		FillPathBuilder& LineTo(const glm::vec2& p1);
		FillPathBuilder& QuadTo(const glm::vec2& p1, const glm::vec2& p2);
		FillPathBuilder& Close();

		FillPath Build() const { return m_Path; }
	private:
		FillPath m_Path;
	};

	struct BBox
	{
		glm::vec2 minPos;
		glm::vec2 maxPos;

		bool DoesIntersect(const BBox& other) const
		{
			if (maxPos.x < other.minPos.x || minPos.x > other.maxPos.x)
				return false;
			if (maxPos.y < other.minPos.y || minPos.y > other.maxPos.y)
				return false;
			return true;
		}
	};

	class FillRenderer
	{
	public:
		void Init();

		void BeginScene(const OrthographicCamera& camera);
		void EndScene();

		void DrawPath(const FillPath& path);
	private:
		void DrawPathInternal(const FillPath& path, float depth = 0.0f);
	private:
		const OrthographicCamera* m_Camera;
		std::vector<FillPath> m_Paths;

		Ref<Shader> m_TriShader;
		Ref<Shader> m_BezShader;
	};

}
