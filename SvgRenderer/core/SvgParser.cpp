#include "SvgParser.h"

#include <tinyxml2.h>

#include <bitset>

namespace SvgRenderer {

	using namespace tinyxml2;

	static std::string_view ExtractFirstNumberString(std::string_view str)
	{
		if (str.empty())
			return str;

		bool readMinus = false;
		bool readDot = false;
		bool readExp = false;

		size_t i = 0;
		for (; i < str.length(); i++)
		{
			switch (str[i])
			{
			case '-':
				if (readMinus)
				{
					if (str[i - 1] != 'e')
						goto end;
				}
				else if (i != 0)
				{
					if (str[i - 1] != 'e')
						goto end;
				}

				readMinus = true;
				break;
			case '.':
				if (readDot)
					goto end;
				readDot = true;
				break;
			case 'e':
				if (readExp)
					goto end;
				readExp = true;
				break;
			default:
			{
				if (!std::isdigit(str[i]))
				{
					goto end;
				}
			}
			}
		}

	end:
		auto ss = str.substr(0, i);
		return ss;
	}

	cppcoro::generator<float> SvgParser::ParseNumbersLine(std::string_view str)
	{
		const char* nums = "0123456789.-e";
		while (!str.empty())
		{
			size_t numIndex = str.find_first_of(nums);
			if (numIndex == std::string_view::npos)
				break;

			str = str.substr(numIndex, std::string_view::npos);

			std::string_view numStr = ExtractFirstNumberString(str);
			std::stringstream ss;
			ss << numStr;
			float value;
			ss >> value;

			co_yield value;

			str = str.substr(numStr.length(), std::string_view::npos);

			//float sign = str[0] == '-' ? -1.0f : 1.0f;
			//if (sign < 1.0f)
			//{
			//	str = str.substr(1, std::string_view::npos);
			//}
			//
			//size_t index = str.find_first_not_of("0123456789.e");
			//if (index != std::string_view::npos && str[index - 1] == 'e')
			//{
			//	index = str.find_first_not_of("0123456789.-e", index);
			//}
			//
			//float value;
			//std::string_view numStr = str.substr(0, index);
			//std::stringstream ss;
			//ss << numStr;
			//ss >> value;
			//
			//co_yield value * sign;
			//
			//if (index == std::string_view::npos)
			//	break;
			//
			//str = str.substr(index, std::string_view::npos);
			//size_t nextIndex = str.find_first_of(nums);
			//if (nextIndex == std::string_view::npos)
			//	break;
			//
			//str = str.substr(nextIndex, std::string_view::npos);
		}
	}

	cppcoro::generator<const std::pair<char, std::vector<float>>&> SvgParser::ParseNumbersUntilOneOf(std::string_view str, std::string_view untilOneOf)
	{
		const char* nums = "0123456789.-";

		str = str.substr(str.find_first_of(untilOneOf), std::string_view::npos);

		while (!str.empty())
		{
			std::pair<char, std::vector<float>> result = std::make_pair(str[0], std::vector<float>());

			str = str.substr(1, std::string_view::npos);

			size_t index = str.find_first_of(untilOneOf);
			if (index == std::string_view::npos)
			{
				if (str.find_first_of(nums) == std::string_view::npos)
				{
					co_yield result;
					break;
				}
				else
				{
					index = str.length();
				}
			}

			std::string_view lineStr = str.substr(0, index);

			size_t numsIndex = lineStr.find_first_of(nums);
			if (numsIndex != std::string_view::npos)
			{
				lineStr = lineStr.substr(numsIndex, std::string_view::npos);
				for (float value : ParseNumbersLine(lineStr))
					result.second.push_back(value);
			}

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
			return {};

		if (colorStr[0] == '#' && colorStr.size() == 4) // #ABC
		{
			uint32_t r, g, b;

			std::stringstream ss;
			ss << std::hex << colorStr[1] << colorStr[1];
			ss >> r;
			ss.clear();
			ss << std::hex << colorStr[2] << colorStr[2];
			ss >> g;
			ss.clear();
			ss << std::hex << colorStr[3] << colorStr[3];
			ss >> b;

			return SvgColor{
				.r = static_cast<uint8_t>(r),
				.g = static_cast<uint8_t>(g),
				.b = static_cast<uint8_t>(b)
			};
		}
		else if (colorStr[0] == '#' && colorStr.size() == 7) // #ABCDEF
		{
			uint32_t r, g, b;

			std::stringstream ss;
			ss << std::hex << colorStr[1] << colorStr[2];
			ss >> r;
			ss.clear();
			ss << std::hex << colorStr[3] << colorStr[4];
			ss >> g;
			ss.clear();
			ss << std::hex << colorStr[5] << colorStr[6];
			ss >> b;

			return SvgColor{
				.r = static_cast<uint8_t>(r),
				.g = static_cast<uint8_t>(g),
				.b = static_cast<uint8_t>(b)
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
		const char* tokens = "MmLlHhVvQqCcZzSsTt";

		std::vector<SvgPath::Segment> segments;

		glm::vec2 prevPoint = { 0.0f, 0.0f };
		glm::vec2 firstPoint = { 0.0f, 0.0f };
		for (const auto& [token, nums] : ParseNumbersUntilOneOf(str, tokens))
		{
			switch (token)
			{
			case 'M':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					{
						glm::vec2 point = { nums[0], nums[1] };
						segments.push_back(SvgPath::Segment(SvgPath::MoveTo{ .p = point }));
						prevPoint = point;
						firstPoint = point;
					}

					for (size_t i = 2; i < nums.size(); i += 2)
					{
						glm::vec2 point = { nums[i], nums[i + 1] };
						segments.push_back(SvgPath::Segment(SvgPath::MoveTo{ .p = point }));
						prevPoint = point;
					}
				}
				else
					SR_WARN("Invalid 'M' in path");
				break;
			case 'm':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					{
						glm::vec2 point = prevPoint + glm::vec2(nums[0], nums[0 + 1]);
						segments.push_back(SvgPath::Segment(SvgPath::MoveTo{ .p = point }));
						prevPoint = point;
						firstPoint = point;
					}

					for (size_t i = 2; i < nums.size(); i += 2)
					{
						glm::vec2 point = prevPoint + glm::vec2(nums[i], nums[i + 1]);
						segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
						prevPoint = point;
					}
				}
				else
					SR_WARN("Invalid 'm' in path");
				break;
			case 'L':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 2)
					{
						glm::vec2 point = { nums[i], nums[i + 1] };
						segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
						prevPoint = point;
					}
				}
				else
					SR_WARN("Invalid 'M' in path");
				break;
			case 'l':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 2)
					{
						glm::vec2 point = prevPoint + glm::vec2(nums[i], nums[i + 1]);
						segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
						prevPoint = point;
					}
				}
				else
					SR_WARN("Invalid 'l' in path");
				break;
			case 'Z':
			case 'z':
				if (nums.empty())
				{
					segments.push_back(SvgPath::Segment(SvgPath::Close{}));
					prevPoint = firstPoint;
				}
				else
					SR_WARN("Invalid 'Z/z' in path");
				break;
			case 'H':
				if (!nums.empty())
				{
					glm::vec2 point = { nums[nums.size() - 1], prevPoint.y };
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'H' in path");
				break;
			case 'h':
				if (!nums.empty())
				{
					float accum = 0.0f;
					for (float n : nums)
						accum += n;

					glm::vec2 point = { prevPoint.x + accum, prevPoint.y };
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'h' in path");
				break;
			case 'V':
				if (!nums.empty())
				{
					glm::vec2 point = { prevPoint.x, nums[nums.size() - 1]};
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'V' in path");
				break;
			case 'v':
				if (!nums.empty())
				{
					float accum = 0.0f;
					for (float n : nums)
						accum += n;

					glm::vec2 point = { prevPoint.x, prevPoint.y + accum };
					segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
					prevPoint = point;
				}
				else
					SR_WARN("Invalid 'v' in path");
				break;
			case 'Q':
				if (!nums.empty() && nums.size() % 4 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 4)
					{
						glm::vec2 p1 = { nums[i + 0], nums[i + 1] };
						glm::vec2 p2 = { nums[i + 2], nums[i + 3] };
						segments.push_back(SvgPath::Segment(SvgPath::QuadTo{ .p1 = p1, .p2 = p2 }));
						prevPoint = p2;
					}
				}
				else
					SR_WARN("Invalid 'Q' in path");
				break;
			case 'q':
				if (!nums.empty() && nums.size() % 4 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 4)
					{
						glm::vec2 p1 = prevPoint + glm::vec2(nums[i + 0], nums[i + 1]);
						glm::vec2 p2 = prevPoint + glm::vec2(nums[i + 2], nums[i + 3]);
						segments.push_back(SvgPath::Segment(SvgPath::QuadTo{ .p1 = p1, .p2 = p2 }));
						prevPoint = p2;
					}
				}
				else
					SR_WARN("Invalid 'q' in path");
				break;
			case 'C':
				if (!nums.empty() && nums.size() % 6 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 6)
					{
						glm::vec2 p1 = { nums[i + 0], nums[i + 1] };
						glm::vec2 p2 = { nums[i + 2], nums[i + 3] };
						glm::vec2 p3 = { nums[i + 4], nums[i + 5] };
						segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
						prevPoint = p3;
					}
				}
				else
					SR_WARN("Invalid 'C' in path");
				break;
			case 'c':
				if (!nums.empty() && nums.size() % 6 == 0)
				{
					for (size_t i = 0; i < nums.size(); i += 6)
					{
						glm::vec2 p1 = prevPoint + glm::vec2(nums[i + 0], nums[i + 1]);
						glm::vec2 p2 = prevPoint + glm::vec2(nums[i + 2], nums[i + 3]);
						glm::vec2 p3 = prevPoint + glm::vec2(nums[i + 4], nums[i + 5]);
						segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
						prevPoint = p3;
					}
				}
				else
					SR_WARN("Invalid 'c' in path");
				break;
			case 'T':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					// TODO: Could be optimized, because after one loop, we know there will be quad segment as the last one
					for (size_t i = 0; i < nums.size(); i += 2)
					{
						if (segments.empty() || segments.back().type != SvgPath::Segment::Type::QuadTo)
						{
							glm::vec2 point = { nums[i], nums[i + 1] };
							segments.push_back(SvgPath::Segment(SvgPath::LineTo{ .p = point }));
							prevPoint = point;
						}
						else
						{
							const SvgPath::QuadTo& prevQuad = segments.back().as.quadTo;
							glm::vec2 p1 = prevPoint + (prevPoint - prevQuad.p1);
							glm::vec2 p2 = { nums[i], nums[i + 1] };
							segments.push_back(SvgPath::Segment(SvgPath::QuadTo{ .p1 = p1, .p2 = p2 }));
							prevPoint = p2;
						}
					}
				}
				else
					SR_WARN("Invalid 'T' in path");
				break;
			case 't':
				if (!nums.empty() && nums.size() % 2 == 0)
				{
					// TODO: Could be optimized, same reason as above
					for (size_t i = 0; i < nums.size(); i += 2)
					{
						if (segments.empty() || segments.back().type != SvgPath::Segment::Type::QuadTo)
						{
							glm::vec2 p1 = prevPoint;
							glm::vec2 p2 = prevPoint + glm::vec2(nums[i], nums[i + 1]);
							segments.push_back(SvgPath::Segment(SvgPath::QuadTo{ .p1 = p1, .p2 = p2 }));
							prevPoint = p2;
						}
						else
						{
							const SvgPath::QuadTo& prevQuad = segments.back().as.quadTo;
							glm::vec2 p1 = prevPoint + (prevPoint - prevQuad.p1);
							glm::vec2 p2 = prevPoint + glm::vec2(nums[i], nums[i + 1]);
							segments.push_back(SvgPath::Segment(SvgPath::QuadTo{ .p1 = p1, .p2 = p2 }));
							prevPoint = p2;
						}
					}
				}
				else
					SR_WARN("Invalid 't' in path");
				break;
			case 'S':
				if (!nums.empty() && nums.size() % 4 == 0)
				{
					// TODO: Could be optimized, same reason as above
					for (size_t i = 0; i < nums.size(); i += 4)
					{
						if (segments.empty() || segments.back().type != SvgPath::Segment::Type::CubicTo)
						{
							glm::vec2 p1 = prevPoint;
							glm::vec2 p2 = { nums[i + 0], nums[i + 1] };
							glm::vec2 p3 = { nums[i + 2], nums[i + 3] };
							segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
							prevPoint = p3;
						}
						else
						{
							const SvgPath::CubicTo& prevCubic = segments.back().as.cubicTo;
							glm::vec2 p1 = prevPoint + (prevPoint - prevCubic.p2);
							glm::vec2 p2 = { nums[i + 0], nums[i + 1] };
							glm::vec2 p3 = { nums[i + 2], nums[i + 3] };
							segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
							prevPoint = p3;
						}
					}
				}
				else
					SR_WARN("Invalid 'S' in path");
				break;
			case 's':
				if (nums.size() % 4 == 0)
				{
					// TODO: Could be optimized, same reason as above
					for (size_t i = 0; i < nums.size(); i += 4)
					{
						if (segments.empty() || segments.back().type != SvgPath::Segment::Type::CubicTo)
						{
							glm::vec2 p1 = prevPoint;
							glm::vec2 p2 = prevPoint + glm::vec2(nums[i + 0], nums[i + 1]);
							glm::vec2 p3 = prevPoint + glm::vec2(nums[i + 2], nums[i + 3]);
							segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
							prevPoint = p3;
						}
						else
						{
							const SvgPath::CubicTo& prevCubic = segments.back().as.cubicTo;
							glm::vec2 p1 = prevPoint + (prevPoint - prevCubic.p2);
							glm::vec2 p2 = prevPoint + glm::vec2(nums[i + 0], nums[i + 1]);
							glm::vec2 p3 = prevPoint + glm::vec2(nums[i + 2], nums[i + 3]);
							segments.push_back(SvgPath::Segment(SvgPath::CubicTo{ .p1 = p1, .p2 = p2, .p3 = p3 }));
							prevPoint = p3;
						}
					}
				}
				else
					SR_WARN("Invalid 's' in path");
				break;
			// TODO: Implement arcs
			}
		}

		return segments;
	}

	SvgNode* SvgParser::ParseSvgGroup(const XMLElement* element, const SvgGroup& previous)
	{
		enum Flag : size_t
		{
			Fill = 0,
			Stroke,
			FillOpacity,
			StrokeOpacity,
			StrokeWidth,
			FillRule,
			Transform,

			FlagCount
		};

		std::bitset<Flag::FlagCount> flags;

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
			else if (attrName == "opacity")
			{
				// TODO: Implement
				SR_WARN("Opacity used inside group, which is not implemented");
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
				if (transform)
				{
					flags.set(Flag::Transform, true);
					group.transform = *transform;
				}
			}
		}

		SvgGroup res;
		res.fill.color = flags.test(Flag::Fill) ? group.fill.color : previous.fill.color;
		res.fill.opacity = flags.test(Flag::FillOpacity) ? group.fill.opacity : previous.fill.opacity;
		res.fill.fillRule = flags.test(Flag::FillRule) ? group.fill.fillRule : previous.fill.fillRule;

		res.stroke.color = flags.test(Flag::Stroke) ? group.stroke.color : previous.stroke.color;
		res.stroke.opacity = flags.test(Flag::StrokeOpacity) ? group.stroke.opacity : previous.stroke.opacity;
		res.stroke.width = flags.test(Flag::StrokeWidth) ? group.stroke.width : previous.stroke.width;
		res.stroke.hasStroke = (flags.test(Flag::Stroke) || previous.stroke.hasStroke) && res.stroke.opacity > 0.0f && res.stroke.width != 0.0f;

		res.transform = flags.test(Flag::Transform) ? group.transform * previous.transform : previous.transform;

		return new SvgNode(res);
	}

	SvgNode* SvgParser::ParseSvgPath(const XMLElement* element, const SvgGroup& group)
	{
		enum Flag : size_t
		{
			Fill = 0,
			Stroke,
			FillOpacity,
			StrokeOpacity,
			StrokeWidth,
			FillRule,
			Transform,
			Segments,

			FlagCount
		};

		std::bitset<Flag::FlagCount> flags;

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
			else if (attrName == "fill-opacity" || attrName == "opacity")
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
				if (transform)
				{
					flags.set(Flag::Transform, true);
					path.transform = *transform;
				}
			}
			else if (attrName == "d")
			{
				path.segments = ParsePathString(attr->Value());
				if (path.segments.empty())
				{
					return nullptr;
				}
			}
		}

		// We return nullptr, if the group did not set any flag, or dont have
		// any segments in the path, because then it does not have any relevant information
		if (flags.none() && path.segments.empty())
		{
			return nullptr;
		}

		path.fill.color = flags.test(Flag::Fill) ? path.fill.color : group.fill.color;
		path.fill.opacity = flags.test(Flag::FillOpacity) ? path.fill.opacity : group.fill.opacity;
		path.fill.fillRule = flags.test(Flag::FillRule) ? path.fill.fillRule : group.fill.fillRule;

		path.stroke.color = flags.test(Flag::Stroke) ? path.stroke.color : group.stroke.color;
		path.stroke.opacity = flags.test(Flag::StrokeOpacity) ? path.stroke.opacity : group.stroke.opacity;
		path.stroke.width = flags.test(Flag::StrokeWidth) ? path.stroke.width : group.stroke.width;
		path.stroke.hasStroke = (flags.test(Flag::Stroke) || group.stroke.hasStroke) && path.stroke.opacity > 0.0f && path.stroke.width != 0.0f;

		path.transform = flags.test(Flag::Transform) ? group.transform * path.transform : group.transform;

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
