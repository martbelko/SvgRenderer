#pragma once

#include "Base.h"
#include "Logger.h"

namespace SvgRenderer {

	class Log
	{
	public:
		static void Init();

		static ref<Logger>& GetLogger() { return s_Logger; }
	private:
		static ref<Logger> s_Logger;
	};

}

#define SR_TRACE(...)         ::SvgRenderer::Log::GetLogger()->Trace(__VA_ARGS__)
#define SR_INFO(...)          ::SvgRenderer::Log::GetLogger()->Info(__VA_ARGS__)
#define SR_WARN(...)          ::SvgRenderer::Log::GetLogger()->Warn(__VA_ARGS__)
#define SR_ERROR(...)         ::SvgRenderer::Log::GetLogger()->Error(__VA_ARGS__)
#define SR_CRITICAL(...)      ::SvgRenderer::Log::GetLogger()->Critical(__VA_ARGS__)
