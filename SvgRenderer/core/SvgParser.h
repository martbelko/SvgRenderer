#pragma once

#include <glm/glm.hpp>

#include <tinyxml2.h>

#include <cppcoro\generator.hpp>

#include <vector>
#include <filesystem>
#include <string_view>
#include <optional>

namespace SvgRenderer {

	enum class SvgNodeType
	{
		Svg = 0, Group, Path
	};

	enum class SvgFillRule
	{
		NonZero = 0, EvenOdd
	};

	struct SvgColor
	{
		uint8_t r, g, b;
	};

	struct SvgFill
	{
		SvgColor color;
		float opacity;
		SvgFillRule fillRule;
	};

	struct SvgStrokeFill
	{
		SvgColor color;
		float opacity;
		float width;
	};

	struct SvgSvg
	{
	};

	struct SvgGroup
	{
		SvgFill fill;
		SvgStrokeFill stroke;
		glm::mat3 transform;

		static SvgGroup CreateDefault()
		{
			return SvgGroup{
				.fill = SvgFill{
					.color = { 0, 0, 0 },
					.opacity = 1.0f,
					.fillRule = SvgFillRule::NonZero
				},
				.stroke = SvgStrokeFill{
					.color = { 0, 0, 0 },
					.opacity = 0.0f,
					.width = 1.0f
				},
				.transform = glm::mat3(1.0f)
			};
		}
	};

	struct SvgPath
	{
		struct MoveTo
		{
			glm::vec2 p;
		};

		struct LineTo
		{
			glm::vec2 p;
		};

		struct QuadTo
		{
			glm::vec2 p1;
			glm::vec2 p2;
		};

		struct CubicTo
		{
			glm::vec2 p1;
			glm::vec2 p2;
			glm::vec2 p3;
		};

		struct Close
		{
		};

		struct Segment
		{
			enum class Type
			{
				MoveTo = 0, LineTo, QuadTo, CubicTo, Close
			};

			Type type;
			union SegmentUnion
			{
				MoveTo moveTo;
				LineTo lineTo;
				QuadTo quadTo;
				CubicTo cubicTo;
				Close close;
			} as;

			Segment(const MoveTo& moveTo)
				: type(Type::MoveTo), as(SegmentUnion{ .moveTo = moveTo }) {}
			Segment(const LineTo& lineTo)
				: type(Type::LineTo), as(SegmentUnion{ .lineTo = lineTo }) {}
			Segment(const QuadTo& quadTo)
				: type(Type::QuadTo), as(SegmentUnion{ .quadTo = quadTo }) {}
			Segment(const CubicTo& cubicTo)
				: type(Type::CubicTo), as(SegmentUnion{ .cubicTo = cubicTo }) {}
			Segment(const Close& close)
				: type(Type::Close), as(SegmentUnion{ .close = close }) {}

			~Segment()
			{
				switch (type)
				{
				case SvgPath::Segment::Type::MoveTo:
					as.moveTo.~MoveTo();
					break;
				case SvgPath::Segment::Type::LineTo:
					as.lineTo.~LineTo();
					break;
				case SvgPath::Segment::Type::QuadTo:
					as.quadTo.~QuadTo();
					break;
				case SvgPath::Segment::Type::CubicTo:
					as.cubicTo.~CubicTo();
					break;
				}
			}
		};

		SvgFill fill;
		SvgStrokeFill stroke;
		glm::mat3 transform;

		std::vector<Segment> segments;
	};

	struct SvgNode
	{
		SvgNodeType type;
		union SvgNodeUnion
		{
			SvgSvg svg;
			SvgGroup group;
			SvgPath path;

			~SvgNodeUnion() {}
		} as;

		std::vector<SvgNode*> children;

		SvgNode(const SvgSvg& svg)
			: type(SvgNodeType::Svg), as(SvgNodeUnion{ .svg = svg }) {}
		SvgNode(const SvgGroup& group)
			: type(SvgNodeType::Group), as(SvgNodeUnion{ .group = group }) {}
		SvgNode(const SvgPath& path)
			: type(SvgNodeType::Path), as(SvgNodeUnion{ .path = path }) {}

		~SvgNode()
		{
			for (SvgNode* node : children)
				delete node;

			switch (type)
			{
			case SvgNodeType::Svg:
				as.svg.~SvgSvg();
				break;
			case SvgNodeType::Path:
				as.path.~SvgPath();
				break;
			case SvgNodeType::Group:
				as.group.~SvgGroup();
				break;
			}
		}
	};

	class SvgParser
	{
	public:
		static SvgNode* Parse(const std::filesystem::path& path);
	private:
		static std::optional<SvgColor> ParseColor(std::string_view colorStr);
		static SvgFillRule ParseFillRule(std::string_view str);
		static std::optional<glm::mat3> ParseTransform(std::string_view str);
		static std::vector<SvgPath::Segment> ParsePathString(std::string_view str);

		static cppcoro::generator<float> ParseNumbersLine(std::string_view str);
		static cppcoro::generator<const std::pair<char, std::vector<float>>&> ParseNumbersUntilOneOf(std::string_view str, std::string_view untilOneOf);

		static SvgNode* ParseSvgGroup(const tinyxml2::XMLElement* element, const SvgGroup& previous);
		static SvgNode* ParseSvgPath(const tinyxml2::XMLElement* element, const SvgGroup& group);

		static void IterateElements(const tinyxml2::XMLElement* element, SvgNode* parent, SvgGroup accumulatedGroup);
	};

}
