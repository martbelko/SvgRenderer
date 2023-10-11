#pragma once

#include "Base.h"
#include "Logger.h"

class Log
{
public:
	static void Init();

	static ref<Logger>& GetLogger() { return s_Logger; }
private:
	static ref<Logger> s_Logger;
};

#define TRACE(...)         ::Log::GetLogger()->Trace(__VA_ARGS__)
#define INFO(...)          ::Log::GetLogger()->Info(__VA_ARGS__)
#define WARN(...)          ::Log::GetLogger()->Warn(__VA_ARGS__)
#define ERROR(...)         ::Log::GetLogger()->Error(__VA_ARGS__)
#define CRITICAL(...)      ::Log::GetLogger()->Critical(__VA_ARGS__)
