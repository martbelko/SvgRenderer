#include "SvgParser.h"

#include <tinyxml2.h>

#include <bitset>

namespace SvgRenderer {

	using namespace tinyxml2;

	cppcoro::generator<float> SvgParser::ParseNumbersLine(std::string_view str)
	{
		const char* nums = "0123456789.-";
		str = str.substr(str.find_first_of(str), std::string_view::npos);

		while (!str.empty())
		{
			size_t index = str.find_first_not_of(nums);
			float value;
			std::string_view numStr = str.substr(0, index);
			std::stringstream ss;
			ss << numStr;
			ss >> value;

			co_yield value;

			str = str.substr(index + 1, std::string_view::npos);
			size_t nextIndex = str.find_first_of(nums);
			if (nextIndex == std::string_view::npos)
				break;

			str = str.substr(nextIndex, std::string_view::npos);
		}
	}

	cppcoro::generator<const std::pair<char, std::vector<float>>&> SvgParser::ParseNumbersUntilOneOf(std::string_view str, std::string_view untilOneOf)
	{
		str = str.substr(str.find_first_of(str), std::string_view::npos);

		while (!str.empty())
		{
			std::pair<char, std::vector<float>> result = std::make_pair(str[0], std::vector<float>());
			str = str.substr(1, std::string_view::npos);
			size_t index = str.find_first_of(untilOneOf);
			if (index == std::string_view::npos)
			{
				co_yield result;
				break;
			}

			auto vv = str.substr(1, index - 1);
			for (float value : ParseNumbersLine(str.substr(1, index - 1)))
				result.second.push_back(value);

			co_yield result;

			size_t nextIndex = str.find_first_of(untilOneOf);
			if (nextIndex == std::string_view::npos)
				break;

			str = str.substr(nextIndex, std::string_view::npos);
		}
	}

	std::optional<SvgColor> SvgParser::ParseColor(std::string_view colorStr)
	{
		if (colorStr.empty())
			return {};

		if (colorStr == "none" || colorStr == "transparent")
		{
			return {};
		}

		if (colorStr[0] == '#' && colorStr.size() == 4) // #ABC
		{
			uint8_t r, g, b;
			std::stringstream ss;
			ss << std::hex << colorStr[1] << colorStr[1] << colorStr[2] << colorStr[2] << colorStr[3] << colorStr[3];
			ss >> std::setw(2) >> r >> std::setw(2) >> g >> std::setw(2) >> b;

			return SvgColor{
				.r = r,
				.g = g,
				.b = b
			};
		}
		else if (colorStr[0] == '#' && colorStr.size() == 7) // #ABCDEF
		{
			uint8_t r, g, b;
			std::stringstream ss;
			ss << std::hex << colorStr.substr(1);
			ss >> std::setw(2) >> r >> std::setw(2) >> g >> std::setw(2) >> b;

			return SvgColor{
				.r = r,
				.g = g,
				.b = b
			};
		}

		SR_WARN("Invalid color");
		return SvgColor{ .r = 0, .g = 0, .b = 0 };
	}

	SvgFillRule SvgParser::ParseFillRule(std::string_view str)
	{
		if (str == "evenodd")
			return SvgFillRule::EvenOdd;
		return SvgFillRule::NonZero;
	}

	std::optional<glm::mat3> SvgParser::ParseTransform(std::string_view str)
	{
		std::string_view cleanString = str.substr(7, str.size() - 8); // matrix(a b c d e f)

		std::vector<float> m;
		m.reserve(6);

		for (float value : ParseNumbersLine(cleanString))
		{
			if (m.size() == 6)
				break;

			m.push_back(value);
		}

		if (m.size() == 6)
		{
			return glm::mat3(
				{ m[0], m[1], 0.0f },
				{ m[2], m[3], 0.0f },
				{ m[4], m[5], 1.0f }
			);
		}
		else
		{
			return {};
		}
	}

	std::vector<SvgPath::Segment> SvgParser::ParsePathString(std::string_view str)
	{
		const char* tokens = "MmLlQqCcZz";

		std::vector<SvgPath::Segment> segments;

		glm::vec2 prevPoint = { 0.0f, 0.0f };
		for (const auto& [token, nums] : ParseNumbersUntilOneOf(str, tokens))
		{
			switch (token)
			{
			case 'M':
				if (nums.size() == 2)
				{
					glm::vec2 point = { nums[0], nums[1] };
					segments.push_back(SvgPath::Segment(SvgPath::MoveTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'M' in path");
				break;
			case 'm':
				if (nums.size() == 2)
				{
					glm::vec2 point = prevPoint + glm::vec2(nums[0], nums[1]);
					segments.push_back(SvgPath::Segment(SvgPath::MoveTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'm' in path");
				break;
			case 'L':
				if (nums.size() == 2)
				{
					glm::vec2 point = { nums[0], nums[1] };
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'M' in path");
				break;
			case 'l':
				if (nums.size() == 2)
				{
					glm::vec2 point = prevPoint + glm::vec2(nums[0], nums[1]);
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'l' in path");
				break;
			// TODO: Implement others
			}
		}

		return segments;
	}

	SvgNode* SvgParser::ParseSvgGroup(const XMLElement* element, const SvgGroup& previous)
	{
		enum Flag : size_t
		{
			Fill = 0, Stroke, FillOpacity, StrokeOpacity, StrokeWidth, FillRule
		};

		std::bitset<8> flags;

		SvgGroup group;

		for (const XMLAttribute* attr = element->FirstAttribute(); attr != nullptr; attr = attr->Next())
		{
			std::string_view attrName = std::string_view(attr->Name());
			if (attrName == "fill")
			{
				std::optional<SvgColor> color = ParseColor(attr->Value());
				if (color)
				{
					flags.set(Flag::Fill, true);
					group.fill.color = *color;
				}
			}
			else if (attrName == "stroke")
			{
				std::optional<SvgColor> color = ParseColor(attr->Value());
				if (color)
				{
					flags.set(Flag::Stroke, true);
					group.stroke.color = *color;
				}
			}
			else if (attrName == "fill-opacity")
			{
				flags.set(Flag::FillOpacity, true);
				group.fill.opacity = attr->FloatValue();
			}
			else if (attrName == "stroke-opacity")
			{
				flags.set(Flag::StrokeOpacity, true);
				group.stroke.opacity = attr->FloatValue();
			}
			else if (attrName == "stroke-width")
			{
				flags.set(Flag::StrokeWidth, true);
				group.stroke.width = attr->FloatValue();
			}
			else if (attrName == "fill-rule")
			{
				flags.set(Flag::FillRule, true);
				group.fill.fillRule = ParseFillRule(attr->Value());
			}
			else if (attrName == "transform")
			{
				std::optional<glm::mat3> transform = ParseTransform(attr->Value());
				group.transform = transform ? *transform : glm::mat3(1.0f);
			}
		}

		// We return nullptr, if the group did not set any flag, because then it does not have any relevant information
		if (flags.none())
			return nullptr;

		SvgGroup res;
		res.fill.color = flags.test(Flag::Fill) ? group.fill.color : previous.fill.color;
		res.fill.opacity = flags.test(Flag::FillOpacity) ? group.fill.opacity : previous.fill.opacity;
		res.fill.fillRule = flags.test(Flag::FillRule) ? group.fill.fillRule : previous.fill.fillRule;

		res.stroke.color = flags.test(Flag::Stroke) ? group.stroke.color : previous.stroke.color;
		res.stroke.opacity = flags.test(Flag::StrokeOpacity) ? group.stroke.opacity : previous.stroke.opacity;

		res.transform = group.transform * previous.transform;

		return new SvgNode(res);
	}

	SvgNode* SvgParser::ParseSvgPath(const XMLElement* element, const SvgGroup& group)
	{
		enum Flag : size_t
		{
			Fill = 0, Stroke, FillOpacity, StrokeOpacity, StrokeWidth, FillRule, Segments
		};

		std::bitset<8> flags;

		SvgPath path;

		for (const XMLAttribute* attr = element->FirstAttribute(); attr != nullptr; attr = attr->Next())
		{
			std::string_view attrName = std::string_view(attr->Name());
			if (attrName == "fill")
			{
				std::optional<SvgColor> color = ParseColor(attr->Value());
				if (color)
				{
					flags.set(Flag::Fill, true);
					path.fill.color = *color;
				}
			}
			else if (attrName == "stroke")
			{
				std::optional<SvgColor> color = ParseColor(attr->Value());
				if (color)
				{
					flags.set(Flag::Stroke, true);
					path.stroke.color = *color;
				}
			}
			else if (attrName == "fill-opacity")
			{
				flags.set(Flag::FillOpacity, true);
				path.fill.opacity = attr->FloatValue();
			}
			else if (attrName == "stroke-opacity")
			{
				flags.set(Flag::StrokeOpacity, true);
				path.stroke.opacity = attr->FloatValue();
			}
			else if (attrName == "stroke-width")
			{
				flags.set(Flag::StrokeWidth, true);
				path.stroke.width = attr->FloatValue();
			}
			else if (attrName == "fill-rule")
			{
				flags.set(Flag::FillRule, true);
				path.fill.fillRule = ParseFillRule(attr->Value());
			}
			else if (attrName == "transform")
			{
				std::optional<glm::mat3> transform = ParseTransform(attr->Value());
				path.transform = transform ? *transform : glm::mat3(1.0f);
			}
			else if (attrName == "d")
			{
				path.segments = ParsePathString(attr->Value());
				if (path.segments.empty())
					return nullptr;
			}
		}

		// We return nullptr, if the group did not set any flag, or dont have
		// any segments in the path, because then it does not have any relevant information
		if (flags.none() || path.segments.empty())
			return nullptr;

		path.fill.color = flags.test(Flag::Fill) ? group.fill.color : group.fill.color;
		path.fill.opacity = flags.test(Flag::FillOpacity) ? group.fill.opacity : group.fill.opacity;
		path.fill.fillRule = flags.test(Flag::FillRule) ? group.fill.fillRule : group.fill.fillRule;

		path.stroke.color = flags.test(Flag::Stroke) ? group.stroke.color : group.stroke.color;
		path.stroke.opacity = flags.test(Flag::StrokeOpacity) ? group.stroke.opacity : group.stroke.opacity;

		path.transform = group.transform * group.transform;

		return new SvgNode(path);
	}

	void SvgParser::IterateElements(const XMLElement* element, SvgNode* parent, SvgGroup accumulatedGroup)
	{
		for (const XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
		{
			SvgNode* node = nullptr;

			std::string_view name = child->Name();
			if (name == "g")
			{
				node = ParseSvgGroup(child, accumulatedGroup);
			}
			else if (name == "path")
			{
				node = ParseSvgPath(child, accumulatedGroup);
			}

			if (node != nullptr)
			{
				parent->children.push_back(node);
				IterateElements(child, node, node->type == SvgNodeType::Group ? node->as.group : accumulatedGroup);
			}
		}
	}

	SvgNode* SvgParser::Parse(const std::filesystem::path& path)
	{
		XMLDocument doc;
		doc.LoadFile(path.string().c_str());

		SvgSvg svg = SvgSvg();
		// TODO: Query things like width, height, viewBox etc into SvgSvg class

		SvgNode* root = new SvgNode(svg);
		SvgGroup accumulatedGroup = SvgGroup::CreateDefault();
		const XMLElement* rootElement = doc.FirstChildElement();

		IterateElements(rootElement, root, accumulatedGroup);
		return root;
	}

}
