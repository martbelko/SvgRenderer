#include "Logger.h"

#include <iomanip>
#include <chrono>
#include <sstream>

namespace SvgRenderer {

	const char* Logger::RESET_CLR = "\x1B[0m";
	const char* Logger::RED_CLR = "\x1B[31m";
	const char* Logger::GREEN_CLR = "\x1B[32m";
	const char* Logger::YELLOW_CLR = "\x1B[33m";
	const char* Logger::RED_BOLD_CLR = "\x1B[31m\u001b[7m";
	const char* Logger::GRAY_CLR = "\x1B[90m";

	std::string Logger::GetTime()
	{
		const auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::stringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
		return ss.str();
	}

}
